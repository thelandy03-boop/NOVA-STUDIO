/*
 * Copyright (C) 2016-2017 Robin Gareus <robin@gareus.org>
 * Copyright (C) 2026 NOVA-STUDIO Team
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <chrono>
#include <algorithm>
#include <vector>
#include <cctype>

#include <glibmm/fileutils.h>
#include <glibmm/markup.h>

#include <ydkmm/pixbuf.h>
#include <ytkmm/stock.h>
#include <ytkmm/separator.h>
#include <ytkmm/scrollbar.h>
#include <ytkmm/messagedialog.h>
#include <ytkmm/filechooserdialog.h>
#include <ytkmm/eventbox.h>
#include <ytkmm/alignment.h>
#include <ytkmm/main.h>

#include "pbd/compose.h"
#include "pbd/file_utils.h"
#include "pbd/gstdio_compat.h"

#include "ardour/filesystem_paths.h"
#include "ardour/luascripting.h"
#include "ardour/filename_extensions.h"

#include "gtkmm2ext/gui_thread.h"
#include "gtkmm2ext/utils.h"

#include "luawindow.h"
#include "luainstance.h"
#include "public_editor.h"
#include "luabridge/LuaBridge.h"
#include "ui_config.h"
#include "ardour_ui.h"

#include "nova_reaper_compat.h"
#include "nova_jsfx_window.h"
#include "nova_jsfx_parser.h"
#include "pbd/i18n.h"

LuaWindow* LuaWindow::_instance = nullptr;

static std::string get_icon_path (const std::string& filename)
{
	std::vector<std::string> search_dirs;
	search_dirs.push_back (Glib::build_filename (ARDOUR::user_config_directory (), "icons", "scripting"));
	search_dirs.push_back (Glib::build_filename ("gtk2_ardour", "icons", "scripting"));
	search_dirs.push_back (Glib::build_filename ("icons", "scripting"));
	search_dirs.push_back (Glib::build_filename ("share", "icons", "scripting"));

	for (const auto& dir : search_dirs) {
		std::string full_path = Glib::build_filename (dir, filename);
		if (Glib::file_test (full_path, Glib::FILE_TEST_EXISTS)) {
			return full_path;
		}
	}
	return "";
}

static Gtk::Image* load_scaled_icon (const std::string& filename, int size = 18)
{
	std::string path = get_icon_path (filename);
	if (!path.empty ()) {
		try {
			Glib::RefPtr<Gdk::Pixbuf> pix = Gdk::Pixbuf::create_from_file (path);
			if (pix) {
				Glib::RefPtr<Gdk::Pixbuf> scaled = pix->scale_simple (size, size, Gdk::INTERP_BILINEAR);
				return Gtk::manage (new Gtk::Image (scaled));
			}
		} catch (...) {}
	}
	return Gtk::manage (new Gtk::Image ());
}

static std::string normalize_path_key (std::string p)
{
	for (char& c : p) {
		if (c == '/') c = '\\';
	}
	if (p.size () >= 2 && p[1] == ':') {
		p[0] = (char) std::toupper ((unsigned char) p[0]);
	}
	return p;
}

LuaWindow::ScriptBuffer::ScriptBuffer (const std::string& name_)
	: name (name_)
	, flags (Buffer_NOFLAG)
	, type (ARDOUR::LuaScriptInfo::Snippet)
{
}

LuaWindow::ScriptBuffer::ScriptBuffer (ARDOUR::LuaScriptInfoPtr info)
	: name (info->name)
	, path (info->path)
	, flags (Buffer_HasFile)
	, type (info->type)
{
}

LuaWindow::ScriptBuffer::~ScriptBuffer () {}

bool LuaWindow::ScriptBuffer::load ()
{
	if (path.empty ()) return false;
	try {
		script = Glib::file_get_contents(path);
		return true;
	} catch (...) {
		return false;
	}
}

LuaWindow* LuaWindow::instance ()
{
	if (!_instance) {
		_instance = new LuaWindow ();
	}
	return _instance;
}

LuaWindow::LuaWindow ()
	: ArdourWindow (_("NOVA-STUDIO - Lua Scripting Environment"))
	, lua (nullptr)
	, _ignore_tab_switch (false)
	, _explorer_visible (false)
{
	setup_ui ();
	reinit_lua ();
	setup_buffers ();

	_btn_run.signal_clicked().connect (sigc::mem_fun (*this, &LuaWindow::run_script));
	_btn_clear.signal_clicked().connect (sigc::mem_fun (*this, &LuaWindow::clear_output));
	_btn_open.signal_clicked().connect (sigc::mem_fun (*this, &LuaWindow::import_script));
	_btn_save.signal_clicked().connect (sigc::mem_fun (*this, &LuaWindow::save_script));
	_btn_delete.signal_clicked().connect (sigc::mem_fun (*this, &LuaWindow::delete_script));
	_btn_revert.signal_clicked().connect (sigc::mem_fun (*this, &LuaWindow::revert_script));
	_btn_explorer.signal_clicked().connect (sigc::mem_fun (*this, &LuaWindow::toggle_explorer));

	/* Explorer */
	_explorer.signal_file_selected().connect (sigc::mem_fun (*this, &LuaWindow::open_file_in_editor));
	_explorer.signal_close_requested().connect (sigc::mem_fun (*this, &LuaWindow::toggle_explorer));
	_explorer.signal_log_message().connect (sigc::mem_fun (*this, &LuaWindow::append_text));
	_explorer.signal_file_renamed().connect (sigc::mem_fun (*this, &LuaWindow::on_file_renamed));

	/* Editor */
	_editor.signal_text_changed().connect(sigc::mem_fun(*this, &LuaWindow::script_changed));
	_editor.signal_cursor_changed().connect(sigc::mem_fun(*this, &LuaWindow::on_cursor_position_changed));
	_editor.signal_lint_status().connect(sigc::mem_fun(*this, &LuaWindow::on_lint_status_changed));

	_tree_inspector.signal_cursor_changed().connect(sigc::mem_fun(*this, &LuaWindow::update_inspector_values));

	/* Tabs nativas */
	_tab_switch_connection = _script_notebook.signal_switch_page().connect (
		sigc::mem_fun (*this, &LuaWindow::on_script_tab_switched));

	set_default_size (1200, 750);
	set_border_width (0);
}

LuaWindow::~LuaWindow ()
{
	delete lua;
}

void LuaWindow::set_btn_icon_and_text (Gtk::Button& btn, const std::string& icon_filename, const std::string& text)
{
	Gtk::HBox* box = Gtk::manage (new Gtk::HBox (false, 4));
	Gtk::Image* img = load_scaled_icon (icon_filename, 18);
	if (img) {
		box->pack_start (*img, Gtk::PACK_SHRINK);
	}
	if (!text.empty ()) {
		Gtk::Label* lbl = Gtk::manage (new Gtk::Label (text));
		box->pack_start (*lbl, Gtk::PACK_SHRINK);
	}
	btn.add (*box);
	box->show_all ();
}

void LuaWindow::build_menubar ()
{
	Gtk::MenuItem* item_file = Gtk::manage (new Gtk::MenuItem (_("File"), false));
	Gtk::Menu* menu_file = Gtk::manage (new Gtk::Menu ());
	item_file->set_submenu (*menu_file);
	_main_menubar.append (*item_file);

	Gtk::MenuItem* item_edit = Gtk::manage (new Gtk::MenuItem (_("Edit"), false));
	_main_menubar.append (*item_edit);

	Gtk::MenuItem* item_run = Gtk::manage (new Gtk::MenuItem (_("Run"), false));
	_main_menubar.append (*item_run);

	Gtk::MenuItem* item_view = Gtk::manage (new Gtk::MenuItem (_("View"), false));
	_main_menubar.append (*item_view);

	Gtk::MenuItem* item_tools = Gtk::manage (new Gtk::MenuItem (_("Tools"), false));
	_main_menubar.append (*item_tools);

	Gtk::MenuItem* item_api = Gtk::manage (new Gtk::MenuItem (_("API Reference"), false));
	_main_menubar.append (*item_api);

	Gtk::MenuItem* item_help = Gtk::manage (new Gtk::MenuItem (_("Help"), false));
	_main_menubar.append (*item_help);
}

void LuaWindow::toggle_explorer ()
{
	_explorer_visible = !_explorer_visible;
	if (_explorer_visible) {
		_explorer.show ();
		_explorer.show_all_children ();
		_explorer.refresh ();
		_editor_hpaned.set_position (260);
	} else {
		_explorer.hide ();
	}
}

void LuaWindow::open_file_in_editor (const std::string& path, const std::string& preset_content)
{
	if (path.empty ()) return;

	const std::string key = normalize_path_key (path);

	for (auto& buf : script_buffers) {
		if (buf && normalize_path_key (buf->path) == key) {
			if (!preset_content.empty ()) {
				buf->script = preset_content;
			} else if (buf->script.empty ()) {
				buf->load ();
			}
			script_selection_changed (buf, true);
			append_text ("> Switched to script: " + path + "\n");
			return;
		}
	}

	std::string content = preset_content;

	if (content.empty ()) {
		if (!Glib::file_test (path, Glib::FILE_TEST_IS_REGULAR)) {
			append_text ("> [Error] File not found: " + path + "\n");
			return;
		}
		for (int attempt = 0; attempt < 8; ++attempt) {
			try {
				content = Glib::file_get_contents (path);
			} catch (...) {}

			if (content.empty ()) {
				std::ifstream file (path.c_str (), std::ios::in | std::ios::binary);
				if (file.is_open ()) {
					std::stringstream ss;
					ss << file.rdbuf ();
					content = ss.str ();
				}
			}

			if (!content.empty ()) break;
			g_usleep (30000);
		}
	}

	ScriptBufferPtr sb (new ScriptBuffer (Glib::path_get_basename (path)));
	sb->path   = path;
	sb->script = content;
	sb->flags  = Buffer_HasFile;

	script_buffers.push_back (sb);
	script_selection_changed (sb, true);

	append_text ("> Switched to script: " + path + "\n");
}

void LuaWindow::on_file_renamed (const std::string& old_path, const std::string& new_path)
{
	const std::string old_key = normalize_path_key (old_path);
	for (ScriptBufferPtr sb : script_buffers) {
		if (!sb) continue;
		if (normalize_path_key (sb->path) == old_key) {
			sb->path = new_path;
			sb->name = Glib::path_get_basename (new_path);
			if (sb == _current_buffer) {
				append_text ("> Buffer path updated: " + new_path + "\n");
			}
		}
	}
	rebuild_tab_strip ();
}

std::string
LuaWindow::tab_title_for (ScriptBufferPtr sb) const
{
	if (!sb) return "?";

	std::string title = sb->name;
	if (title.empty () && !sb->path.empty ()) {
		title = Glib::path_get_basename (sb->path);
	}
	if (title.empty ()) {
		title = _("Untitled");
	}

	if (title.size () > 4) {
		std::string lower = title;
		std::transform (lower.begin (), lower.end (), lower.begin (), ::tolower);
		if (lower.substr (lower.size () - 4) == ".lua") {
			title = title.substr (0, title.size () - 4);
		}
	}

	if (sb->flags & Buffer_Dirty) {
		return std::string ("• ") + title;
	}
	return title;
}

int
LuaWindow::page_index_for_buffer (ScriptBufferPtr sb) const
{
	if (!sb) return -1;
	for (int i = 0; i < (int) script_buffers.size (); ++i) {
		if (script_buffers[i] == sb) return i;
	}
	return -1;
}

Gtk::Widget*
LuaWindow::make_tab_label (ScriptBufferPtr sb)
{
	Gtk::HBox* box = Gtk::manage (new Gtk::HBox (false, 4));
	Gtk::Label* lbl = Gtk::manage (new Gtk::Label (tab_title_for (sb)));
	Gtk::Button* close_btn = Gtk::manage (new Gtk::Button ("×"));

	close_btn->set_relief (Gtk::RELIEF_NONE);
	close_btn->set_focus_on_click (false);
	close_btn->set_size_request (16, 16);
	close_btn->set_tooltip_text (_("Close"));

	close_btn->signal_clicked().connect (
		sigc::bind (sigc::mem_fun (*this, &LuaWindow::close_tab), sb));

	box->pack_start (*lbl, true, true, 0);
	box->pack_start (*close_btn, false, false, 0);
	box->show_all ();
	return box;
}

void
LuaWindow::rebuild_tab_strip ()
{
	_tab_switch_connection.block ();
	_ignore_tab_switch = true;

	while ((size_t)_script_notebook.get_n_pages() > script_buffers.size()) {
		_script_notebook.remove_page(_script_notebook.get_n_pages() - 1);
	}

	while ((size_t)_script_notebook.get_n_pages() < script_buffers.size()) {
		Gtk::Label* placeholder = Gtk::manage(new Gtk::Label());
		placeholder->set_size_request(0, 0);
		size_t idx = _script_notebook.get_n_pages();
		_script_notebook.append_page(*placeholder, *make_tab_label(script_buffers[idx]));
	}

	for (size_t i = 0; i < script_buffers.size(); ++i) {
		Gtk::Widget* page = _script_notebook.get_nth_page(i);
		if (page) {
			_script_notebook.set_tab_label(*page, *make_tab_label(script_buffers[i]));
		}
	}

	int cur_idx = page_index_for_buffer(_current_buffer);
	if (cur_idx >= 0) {
		_script_notebook.set_current_page(cur_idx);
	}

	_script_notebook.show_all ();

	while (Gtk::Main::events_pending()) {
		Gtk::Main::iteration();
	}

	_ignore_tab_switch = false;
	_tab_switch_connection.unblock ();
}

void
LuaWindow::on_script_tab_switched (GtkNotebookPage* /*page*/, guint page_num)
{
	if (_ignore_tab_switch) return;
	if (page_num >= script_buffers.size ()) return;

	ScriptBufferPtr sb = script_buffers[page_num];
	if (!sb || sb == _current_buffer) return;

	if (_current_buffer) {
		_current_buffer->script = _editor.get_text ();
	}

	_current_buffer = sb;
	_editor.set_text (sb->script);

	append_text ("> Switched to script: " + (sb->path.empty () ? sb->name : sb->path) + "\n");
}

void
LuaWindow::close_tab (ScriptBufferPtr sb)
{
	if (!sb) return;

	if (sb->flags & Buffer_Dirty) {
		Gtk::MessageDialog dialog (
			*this,
			string_compose (_("'%1' has unsaved changes. Close anyway?"), tab_title_for (sb)),
			false,
			Gtk::MESSAGE_WARNING,
			Gtk::BUTTONS_NONE,
			true);
		dialog.add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
		dialog.add_button (_("Close without Saving"), Gtk::RESPONSE_ACCEPT);
		dialog.add_button (Gtk::Stock::SAVE, Gtk::RESPONSE_YES);
		dialog.set_default_response (Gtk::RESPONSE_YES);
		dialog.set_position (Gtk::WIN_POS_CENTER);

		int resp = dialog.run ();
		dialog.hide ();

		if (resp == Gtk::RESPONSE_CANCEL) {
			return;
		}
		if (resp == Gtk::RESPONSE_YES) {
			ScriptBufferPtr prev = _current_buffer;
			script_selection_changed (sb, true);
			save_script ();
			if (prev && prev != sb) {
				script_selection_changed (prev, true);
			}
			if (sb->flags & Buffer_Dirty) {
				return;
			}
		}
	}

	ScriptBufferList::iterator it = std::find (script_buffers.begin (), script_buffers.end (), sb);
	if (it == script_buffers.end ()) return;

	bool was_current = (sb == _current_buffer);
	script_buffers.erase (it);

	if (script_buffers.empty ()) {
		ScriptBufferPtr scratch (new ScriptBuffer (_("Scratch Buffer #1")));
		scratch->script =
			"---- this header is required to save the script\n"
			"-- ardour { [\"type\"] = \"Snippet\", name = \"Scratch\", author = \"NOVA\" }\n\n"
			"function factory ()\n"
			"    return function ()\n"
			"        local session = Session:instance()\n"
			"    end\n"
			"end\n";
		scratch->flags = Buffer_Scratch;
		script_buffers.push_back (scratch);
		script_selection_changed (scratch, true);
	} else if (was_current) {
		script_selection_changed (script_buffers.back (), true);
	} else {
		rebuild_tab_strip ();
	}
}

void LuaWindow::setup_ui ()
{
	Gtk::VBox* main_vbox = Gtk::manage (new Gtk::VBox (false, 0));
	main_vbox->set_border_width (0);
	add (*main_vbox);

	// 1. Menú Superior
	build_menubar ();
	main_vbox->pack_start (_main_menubar, false, true, 0);

	// 2. Toolbar Superior
	Gtk::HBox* toolbar = Gtk::manage (new Gtk::HBox (false, 4));
	toolbar->set_border_width (4);

	set_btn_icon_and_text (_btn_run, "play32.png", _("Run"));
	set_btn_icon_and_text (_btn_pause, "pause32.png", _("Pause"));
	set_btn_icon_and_text (_btn_stop, "stop32.png", _("Stop"));
	set_btn_icon_and_text (_btn_autorun, "autorun32.png", _("Auto-run"));

	toolbar->pack_start (_btn_run, Gtk::PACK_SHRINK);
	toolbar->pack_start (_btn_pause, Gtk::PACK_SHRINK);
	toolbar->pack_start (_btn_stop, Gtk::PACK_SHRINK);
	toolbar->pack_start (_btn_autorun, Gtk::PACK_SHRINK);

	toolbar->pack_start (*Gtk::manage (new Gtk::VSeparator ()), Gtk::PACK_SHRINK);

	_api_search_entry.set_text (_("Filter API / Functions..."));
	_api_search_entry.set_width_chars (24);
	toolbar->pack_start (_api_search_entry, Gtk::PACK_SHRINK);

	toolbar->pack_start (*Gtk::manage (new Gtk::VSeparator ()), Gtk::PACK_SHRINK);

	set_btn_icon_and_text (_btn_open, "folder32.png", _("Load"));
	set_btn_icon_and_text (_btn_save, "Save.png", _("Save"));
	set_btn_icon_and_text (_btn_delete, "delete32.png", _("Delete"));
	set_btn_icon_and_text (_btn_options, "options.png", _("Options"));
	set_btn_icon_and_text (_btn_explorer, "folder32.png", _("Explorer"));

	toolbar->pack_start (_btn_open, Gtk::PACK_SHRINK);
	toolbar->pack_start (_btn_save, Gtk::PACK_SHRINK);
	toolbar->pack_start (_btn_delete, Gtk::PACK_SHRINK);
	toolbar->pack_start (_btn_options, Gtk::PACK_SHRINK);
	toolbar->pack_start (_btn_explorer, Gtk::PACK_SHRINK);

	main_vbox->pack_start (*toolbar, Gtk::PACK_SHRINK);

	// 3. Tabs nativas (Notebook) + Editor
	_script_notebook.set_scrollable (true);
	_script_notebook.set_show_border (false);
	_script_notebook.set_tab_pos (Gtk::POS_TOP);

	Gtk::VBox* editor_container = Gtk::manage (new Gtk::VBox (false, 0));
	editor_container->pack_start (_script_notebook, Gtk::PACK_SHRINK);
	editor_container->pack_start (_editor, Gtk::PACK_EXPAND_WIDGET);

	// 4. Inspector derecha
	Gtk::VBox* inspector_vbox = Gtk::manage (new Gtk::VBox (false, 6));
	inspector_vbox->set_border_width (8);
	inspector_vbox->set_size_request (260, -1);

	Gtk::Label* lbl_insp = Gtk::manage (new Gtk::Label ());
	lbl_insp->set_markup ("<b>Variable / Debug Inspector</b>");
	lbl_insp->set_alignment (0.0, 0.5);
	inspector_vbox->pack_start (*lbl_insp, Gtk::PACK_SHRINK);

	_model_inspector = Gtk::ListStore::create (_inspector_cols);
	_tree_inspector.set_model (_model_inspector);
	_tree_inspector.set_headers_visible (true);

	while (_tree_inspector.get_columns().size() > 0) {
		_tree_inspector.remove_column (*_tree_inspector.get_column(0));
	}
	_tree_inspector.append_column (_("Name"), _inspector_cols.col_name);
	_tree_inspector.append_column (_("Value"), _inspector_cols.col_value);

	if (Gtk::TreeViewColumn* c0 = _tree_inspector.get_column (0)) {
		c0->set_expand (true);
		c0->set_resizable (true);
		c0->set_min_width (100);
	}
	if (Gtk::TreeViewColumn* c1 = _tree_inspector.get_column (1)) {
		c1->set_expand (true);
		c1->set_resizable (true);
		c1->set_min_width (110);
	}

	Gtk::ScrolledWindow* scroll_insp = Gtk::manage (new Gtk::ScrolledWindow ());
	scroll_insp->set_policy (Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	scroll_insp->set_shadow_type (Gtk::SHADOW_IN);
	scroll_insp->add (_tree_inspector);
	inspector_vbox->pack_start (*scroll_insp, Gtk::PACK_EXPAND_WIDGET);

	Gtk::HSeparator* sep_qw = Gtk::manage (new Gtk::HSeparator ());
	inspector_vbox->pack_start (*sep_qw, Gtk::PACK_SHRINK);

	Gtk::Label* lbl_qw = Gtk::manage (new Gtk::Label ());
	lbl_qw->set_markup ("<b>Quick Watches</b>");
	lbl_qw->set_alignment (0.0, 0.5);
	inspector_vbox->pack_start (*lbl_qw, Gtk::PACK_SHRINK);

	_btn_add_watch.set_label (_("+ Add Watch"));
	inspector_vbox->pack_start (_btn_add_watch, Gtk::PACK_SHRINK);

	_top_hpaned.pack1 (*editor_container, true, true);
	_top_hpaned.pack2 (*inspector_vbox, false, false);
	_top_hpaned.set_position (780);

	_editor_hpaned.pack1 (_explorer, false, false);
	_editor_hpaned.pack2 (_top_hpaned, true, true);
	_editor_hpaned.set_position (260);

	// 5. Panel Consola (Bottom Notebook)
	scrollout.add (outtext);
	scrollout.set_policy (Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	outtext.set_editable (false);

	Gtk::VBox* console_vbox = Gtk::manage (new Gtk::VBox (false, 4));
	console_vbox->pack_start (scrollout, Gtk::PACK_EXPAND_WIDGET);

	Gtk::HBox* console_btns = Gtk::manage (new Gtk::HBox (false, 4));
	_btn_clear.set_label (_("Clear Output"));
	_btn_revert.set_label (_("Revert Script"));
	console_btns->pack_start (_btn_clear, Gtk::PACK_SHRINK);
	console_btns->pack_start (_btn_revert, Gtk::PACK_SHRINK);
	console_vbox->pack_start (*console_btns, Gtk::PACK_SHRINK);

	_notebook_bottom.append_page (*console_vbox, _("Script Editor Output"));
	_notebook_bottom.append_page (_console, _("Interactive Console"));
	_notebook_bottom.append_page (*Gtk::manage (new Gtk::Label (_("API Documentation Explorer"))), _("API Docs Explorer"));

	_main_vpaned.pack1 (_editor_hpaned, true, true);
	_main_vpaned.pack2 (_notebook_bottom, false, true);

	main_vbox->pack_start (_main_vpaned, Gtk::PACK_EXPAND_WIDGET);

	// 6. Barra de Estado
	Gtk::HBox* statusbar = Gtk::manage (new Gtk::HBox (false, 8));
	statusbar->set_border_width (2);

	_lbl_status_pos.set_text (_("Line 1, Col 1"));
	_lbl_lua_ver.set_text (_("Engine: Lua 5.3 (Scintilla C++ Engine)"));

	statusbar->pack_start (_lbl_status_pos, Gtk::PACK_SHRINK);
	statusbar->pack_end (_lbl_lua_ver, Gtk::PACK_SHRINK);

	main_vbox->pack_start (*statusbar, Gtk::PACK_SHRINK);

	update_inspector_values ();
	main_vbox->show_all ();

	_explorer.set_no_show_all(true);
	_explorer.hide ();
	_explorer_visible = false;
}

void LuaWindow::update_inspector_values ()
{
	_model_inspector->clear ();

	Gtk::TreeModel::Row row = *(_model_inspector->append ());
	row[_inspector_cols.col_name] = "session";
	if (_session) {
		char ptr_buf[64];
		snprintf (ptr_buf, sizeof(ptr_buf), "0x%p", (void*)_session);
		row[_inspector_cols.col_value] = ptr_buf;
	} else {
		row[_inspector_cols.col_value] = "nil";
	}

	row = *(_model_inspector->append ());
	row[_inspector_cols.col_name] = "sel_regions";
	row[_inspector_cols.col_value] = "<Array>";

	row = *(_model_inspector->append ());
	row[_inspector_cols.col_name] = "is_playing";
	row[_inspector_cols.col_value] = (_session && _session->transport_rolling()) ? "true" : "false";

	row = *(_model_inspector->append ());
	row[_inspector_cols.col_name] = "playhead_pos";
	row[_inspector_cols.col_value] = _session ? std::to_string(_session->transport_sample()) : "00:00:00:00";

	row = *(_model_inspector->append ());
	row[_inspector_cols.col_name] = "track_count";
	row[_inspector_cols.col_value] = _session ? std::to_string(_session->get_tracks()->size()) : "0";

	row = *(_model_inspector->append ());
	row[_inspector_cols.col_name] = "mouse_x";
	row[_inspector_cols.col_value] = "1142";

	row = *(_model_inspector->append ());
	row[_inspector_cols.col_name] = "mouse_y";
	row[_inspector_cols.col_value] = "582";
}

void LuaWindow::on_cursor_position_changed ()
{
	int line = 1, col = 1;
	_editor.get_cursor_position (line, col);

	char tmp[64];
	snprintf(tmp, sizeof(tmp), "Line %d, Col %d", line, col);
	_lbl_status_pos.set_text(tmp);
}

void LuaWindow::script_changed ()
{
	if (_current_buffer) {
		_current_buffer->script = _editor.get_text ();
		_current_buffer->flags = (BufferFlags)(_current_buffer->flags | Buffer_Dirty);
		int idx = page_index_for_buffer (_current_buffer);
		if (idx >= 0) {
			Gtk::Widget* page = _script_notebook.get_nth_page (idx);
			if (page) {
				_script_notebook.set_tab_label (*page, *make_tab_label (_current_buffer));
			}
		}
	}
}

void LuaWindow::setup_buffers ()
{
	ScriptBufferPtr sb (new ScriptBuffer (_("Scratch Buffer #1")));

	std::string raw_script =
		"---- this header is required to save the script\n"
		"-- ardour { [\"type\"] = \"Snippet\", name = \"Advanced Align & Split\", author = \"NOVA\" }\n\n"
		"function factory ()\n"
		"    return function ()\n"
		"        local session = Session:instance()\n"
		"        local sel_regions = Editor:get_selection().regions\n\n"
		"        for r in sel_regions:iter() do\n"
		"            local pos = r:position()\n"
		"            if pos > 0 then\n"
		"                r:set_position(pos + 1000)\n"
		"            end\n"
		"        end\n"
		"    end\n"
		"end\n";

	sb->script = raw_script;
	sb->flags = Buffer_Scratch;
	script_buffers.push_back (sb);
	script_selection_changed (sb, true);
}

void LuaWindow::script_selection_changed (ScriptBufferPtr sb, bool /*force*/)
{
	if (!sb) return;

	if (_current_buffer && _current_buffer != sb) {
		_current_buffer->script = _editor.get_text ();
	}

	_current_buffer = sb;
	_editor.set_text (sb->script);

	rebuild_tab_strip ();
}

void LuaWindow::reinit_lua ()
{
	ENSURE_GUI_THREAD (*this, &LuaWindow::session_going_away);
	delete lua;
	lua = new LuaState (true, UIConfiguration::instance().get_sandbox_all_lua_scripts ());
	lua->Print.connect (sigc::mem_fun (*this, &LuaWindow::append_text));

	lua_State* L = lua->getState();
	LuaInstance::register_classes (L, UIConfiguration::instance().get_sandbox_all_lua_scripts ());
	luabridge::push <PublicEditor *> (L, &PublicEditor::instance());
	lua_setglobal (L, "Editor");

	if (_session) {
		luabridge::push <ARDOUR::Session *> (L, _session);
		lua_setglobal (L, "Session");

		luaL_dostring(L, "if Session then "
		                 "  local s = Session; "
		                 "  Session = { instance = function() return s end }; "
		                 "  setmetatable(Session, { __index = s }); "
		                 "end");
	}

	// Inyección limpia del módulo ReaScript independiente
	NovaReaperCompat::inject(L);

	_editor.set_lua_state(L);
	_console.set_lua_state(L);
}

void LuaWindow::run_script ()
{
	std::string script_text = _editor.get_text ();

	if (script_text.empty()) return;

	// Flujo Mágico JSFX: Desplegar Ventana Flotante GUI Instantáneamente
	if (NovaJSFXParser::is_jsfx_code (script_text)) {
		append_text ("> [JSFX Engine] Lanzando Ventana Flotante de Interfaz Gráfica...\n");
		
		// Desplegar la GUI Emergente con los Sliders
		NovaJSFXWindow::launch_for_jsfx(script_text);

		append_text ("> [OK] ¡Interfaz Emergente desplegada en pantalla con controles en tiempo real!\n");
		return;
	}

	append_text (_("> Executing Lua script...\n"));
	auto start_time = std::chrono::high_resolution_clock::now();

	reinit_lua ();
	if (!lua) return;

	_editor.invalidate_globals_cache();

	lua_State* L = lua->getState();

	int err = luaL_dostring (L, script_text.c_str ());

	auto end_time = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> duration = end_time - start_time;

	if (err == 0) {
		char finished_msg[128];
		snprintf(finished_msg, sizeof(finished_msg), "> [Lua] Execution finished in %.3f seconds.\n", duration.count() / 1000.0);
		append_text (_(finished_msg));
	} else {
		const char* lua_err = lua_tostring (L, -1);
		std::string err_str = "> [Lua Error] ";
		err_str += (lua_err ? lua_err : "Unknown execution error");
		err_str += "\n";
		append_text (err_str);
		lua_pop (L, 1);
	}
	update_inspector_values();
}
void LuaWindow::append_text (std::string s)
{
	Glib::RefPtr<Gtk::TextBuffer> tb (outtext.get_buffer());
	tb->insert (tb->end(), s);
	scroll_to_bottom ();
	if (Gtkmm2ext::UI::instance()) {
		Gtkmm2ext::UI::instance()->flush_pending (0.05);
	}
}

void LuaWindow::scroll_to_bottom ()
{
	Gtk::Adjustment* adj = scrollout.get_vadjustment ();
	if (adj) {
		adj->set_value (adj->get_upper () - adj->get_page_size ());
	}
}

void LuaWindow::clear_output ()
{
	outtext.get_buffer ()->set_text ("");
}

void LuaWindow::import_script ()
{
	Gtk::FileChooserDialog dialog(*this, _("Choose a Lua Script"), Gtk::FILE_CHOOSER_ACTION_OPEN);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_ACCEPT);

	Gtk::FileFilter filter_lua;
	filter_lua.set_name(_("Lua Scripts (*.lua)"));
	filter_lua.add_pattern("*.lua");
	dialog.add_filter(filter_lua);

	Gtk::FileFilter filter_any;
	filter_any.set_name(_("All Files"));
	filter_any.add_pattern("*");
	dialog.add_filter(filter_any);

	if (dialog.run() == Gtk::RESPONSE_ACCEPT) {
		std::string path = dialog.get_filename();
		open_file_in_editor (path);
	}
}

void LuaWindow::save_script ()
{
	if (!_current_buffer) return;

	if (_current_buffer->path.empty()) {
		Gtk::FileChooserDialog dialog(*this, _("Save Script As"), Gtk::FILE_CHOOSER_ACTION_SAVE);
		dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
		dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_ACCEPT);
		dialog.set_do_overwrite_confirmation(true);

		Gtk::FileFilter filter_lua;
		filter_lua.set_name(_("Lua Scripts (*.lua)"));
		filter_lua.add_pattern("*.lua");
		dialog.add_filter(filter_lua);

		std::string suggest = _current_buffer->name;
		if (suggest.size () < 4 || suggest.substr (suggest.size () - 4) != ".lua") {
			suggest += ".lua";
		}
		dialog.set_current_name (suggest);

		if (dialog.run() == Gtk::RESPONSE_ACCEPT) {
			_current_buffer->path = dialog.get_filename();
			_current_buffer->name = Glib::path_get_basename(_current_buffer->path);
		} else {
			return;
		}
	}

	try {
		Glib::file_set_contents(_current_buffer->path, _editor.get_text());
		_current_buffer->script = _editor.get_text();
		_current_buffer->flags = (BufferFlags)(_current_buffer->flags & ~Buffer_Dirty);
		_current_buffer->flags = (BufferFlags)(_current_buffer->flags | Buffer_HasFile);
		_current_buffer->flags = (BufferFlags)(_current_buffer->flags & ~Buffer_Scratch);
		append_text("> Script saved successfully to: " + _current_buffer->path + "\n");
		rebuild_tab_strip ();
		if (_explorer_visible) {
			_explorer.refresh ();
		}
	} catch (...) {
		append_text("> [Error] Failed to write script: " + _current_buffer->path + "\n");
	}
}

void LuaWindow::delete_script ()
{
	if (_current_buffer) {
		close_tab (_current_buffer);
	}
}

void LuaWindow::revert_script ()
{
	if (!_current_buffer || _current_buffer->path.empty()) return;

	if (_current_buffer->load()) {
		_editor.set_text(_current_buffer->script);
		_current_buffer->flags = (BufferFlags)(_current_buffer->flags & ~Buffer_Dirty);
		rebuild_tab_strip ();
		append_text("> Reverted editor to last saved file state.\n");
	}
}

void LuaWindow::set_session (ARDOUR::Session* s)
{
	SessionHandlePtr::set_session(s);
	update_inspector_values();
	reinit_lua(); // Actualizar el puntero Session en el motor Lua
	if (_explorer_visible) {
		_explorer.refresh ();
	}
}

void LuaWindow::session_going_away ()
{
	SessionHandlePtr::session_going_away();
	update_inspector_values();
}

void LuaWindow::edit_script (const std::string&, const std::string&) {}
void LuaWindow::update_title () {}
void LuaWindow::refresh_scriptlist () {}
void LuaWindow::rebuild_menu () {}
void LuaWindow::update_gui_state () {}

uint32_t LuaWindow::count_scratch_buffers () const
{
	uint32_t n = 0;
	for (const auto& b : script_buffers) {
		if (!b) continue;
		if ((b->flags & Buffer_Scratch) || b->path.empty ()) {
			++n;
		}
	}
	return n;
}

void LuaWindow::new_script ()
{
	uint32_t n = count_scratch_buffers () + 1;
	char buf[64];
	snprintf (buf, sizeof (buf), "Scratch Buffer #%u", n);

	ScriptBufferPtr sb (new ScriptBuffer (buf));
	sb->script =
		"---- this header is required to save the script\n"
		"-- ardour { [\"type\"] = \"Snippet\", name = \"Scratch\", author = \"NOVA\" }\n\n"
		"function factory ()\n"
		"    return function ()\n"
		"        local session = Session:instance()\n"
		"        -- Your code here\n"
		"    end\n"
		"end\n";
	sb->flags = Buffer_Scratch;

	script_buffers.push_back (sb);
	script_selection_changed (sb, true);
	append_text (std::string ("> New scratch: ") + buf + "\n");
}

static std::string xml_escape_text (const std::string& text)
{
	std::string res;
	res.reserve (text.size ());
	for (char c : text) {
		switch (c) {
			case '<':  res += "&lt;"; break;
			case '>':  res += "&gt;"; break;
			case '&':  res += "&amp;"; break;
			case '"':  res += "&quot;"; break;
			case '\'': res += "&apos;"; break;
			default:   res += c; break;
		}
	}
	return res;
}

void LuaWindow::on_lint_status_changed (bool ok, std::string msg, int line)
{
	if (ok) {
		_lbl_lua_ver.set_markup ("<span foreground='#00F0FF'>✔ Syntax OK</span>  |  Engine: Lua 5.3 (Scintilla C++ Engine)");
	} else {
		std::string clean_msg = xml_escape_text (msg);
		char buf[512];
		snprintf (buf, sizeof(buf), "<span foreground='#FF3366'>✖ Line %d: %s</span>  |  Engine: Lua 5.3", line, clean_msg.c_str());
		_lbl_lua_ver.set_markup (buf);
	}
}