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

#include <ydkmm/pixbuf.h>
#include <ytkmm/stock.h>
#include <ytkmm/separator.h>
#include <ytkmm/scrollbar.h>
#include <ytkmm/messagedialog.h>
#include <ytkmm/filechooserdialog.h>

#include "pbd/file_utils.h"
#include "pbd/gstdio_compat.h"

#include "ardour/filesystem_paths.h"
#include "ardour/luascripting.h"

#include "gtkmm2ext/gui_thread.h"
#include "gtkmm2ext/utils.h"

#include "luawindow.h"
#include "luainstance.h"
#include "public_editor.h"
#include "luabridge/LuaBridge.h"
#include "ui_config.h"
#include "ardour_ui.h"

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
	std::ifstream file (path.c_str ());
	if (!file.is_open ()) return false;
	std::stringstream strStream;
	strStream << file.rdbuf ();
	script = strStream.str ();
	return true;
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
{
	setup_ui ();
	setup_buffers ();

	_btn_run.signal_clicked().connect (sigc::mem_fun (*this, &LuaWindow::run_script));
	_btn_clear.signal_clicked().connect (sigc::mem_fun (*this, &LuaWindow::clear_output));
	_btn_open.signal_clicked().connect (sigc::mem_fun (*this, &LuaWindow::import_script));
	_btn_save.signal_clicked().connect (sigc::mem_fun (*this, &LuaWindow::save_script));
	_btn_delete.signal_clicked().connect (sigc::mem_fun (*this, &LuaWindow::delete_script));
	_btn_revert.signal_clicked().connect (sigc::mem_fun (*this, &LuaWindow::revert_script));

	editor.signal_text_changed().connect(sigc::mem_fun(*this, &LuaWindow::script_changed));
	editor.signal_cursor_changed().connect(sigc::mem_fun(*this, &LuaWindow::on_cursor_position_changed));

	_tree_inspector.signal_cursor_changed().connect(sigc::mem_fun(*this, &LuaWindow::update_inspector_values));

	set_default_size (1080, 720);
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

	set_btn_icon_and_text (_btn_open, "folder.png", _("Load"));
	set_btn_icon_and_text (_btn_save, "Save.png", _("Save"));
	set_btn_icon_and_text (_btn_delete, "delete32.png", _("Delete"));
	set_btn_icon_and_text (_btn_options, "options.png", _("Options"));

	toolbar->pack_start (_btn_open, Gtk::PACK_SHRINK);
	toolbar->pack_start (_btn_save, Gtk::PACK_SHRINK);
	toolbar->pack_start (_btn_delete, Gtk::PACK_SHRINK);
	toolbar->pack_start (_btn_options, Gtk::PACK_SHRINK);

	main_vbox->pack_start (*toolbar, Gtk::PACK_SHRINK);

	// 3. Panel Editor (Scintilla C++ IDE Engine Empaquetado Directamente)
	Gtk::HBox* editor_box = Gtk::manage (new Gtk::HBox (false, 0));
	editor_box->pack_start (editor, Gtk::PACK_EXPAND_WIDGET);

	Gtk::Notebook* editor_tabs = Gtk::manage (new Gtk::Notebook ());
	editor_tabs->append_page (*editor_box, _("📄 Script Editor  ✕"));

	// Inspector Derecha
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

	_top_hpaned.pack1 (*editor_tabs, true, true);
	_top_hpaned.pack2 (*inspector_vbox, false, false);
	_top_hpaned.set_position (780);

	// 4. Panel Consola
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

	_notebook_bottom.append_page (*console_vbox, _("Script Editor"));
	_notebook_bottom.append_page (*Gtk::manage (new Gtk::Label (_("Interactive Lua Console ready..."))), _("Interactive Console"));
	_notebook_bottom.append_page (*Gtk::manage (new Gtk::Label (_("API Documentation Explorer"))), _("API Docs Explorer"));

	_main_vpaned.pack1 (_top_hpaned, true, true);
	_main_vpaned.pack2 (_notebook_bottom, false, true);

	main_vbox->pack_start (_main_vpaned, Gtk::PACK_EXPAND_WIDGET);

	// 5. Barra de Estado
	Gtk::HBox* statusbar = Gtk::manage (new Gtk::HBox (false, 8));
	statusbar->set_border_width (2);

	_lbl_status_pos.set_text (_("Line 1, Col 1"));
	_lbl_lua_ver.set_text (_("Engine: Lua 5.3 (Scintilla C++ Engine)"));

	statusbar->pack_start (_lbl_status_pos, Gtk::PACK_SHRINK);
	statusbar->pack_end (_lbl_lua_ver, Gtk::PACK_SHRINK);

	main_vbox->pack_start (*statusbar, Gtk::PACK_SHRINK);

	update_inspector_values ();
	main_vbox->show_all ();
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
	editor.get_cursor_position (line, col);

	char tmp[64];
	snprintf(tmp, sizeof(tmp), "Line %d, Col %d", line, col);
	_lbl_status_pos.set_text(tmp);
}

void LuaWindow::script_changed ()
{
	if (_current_buffer) {
		_current_buffer->script = editor.get_text ();
		_current_buffer->flags = (BufferFlags)(_current_buffer->flags | Buffer_Dirty);
	}
}

void LuaWindow::setup_buffers ()
{
	ScriptBufferPtr sb (new ScriptBuffer (_("Scratch Buffer #1")));

	std::string raw_script = "---- this header is required to save the script\n-- ardour { [\"type\"] = \"Snippet\", name = \"Advanced Align & Split\", author = \"NOVA\" }\n\nfunction factory ()\n    return function ()\n        local session = Session:instance()\n        local sel_regions = Editor:get_selection().regions\n\n        for r in sel_regions:iter() do\n            local pos = r:position()\n            if pos > 0 then\n                r:set_position(pos + 1000)\n            end\n        end\n    end\nend\n";

	sb->script = raw_script;
	script_buffers.push_back (sb);
	script_selection_changed (sb, true);
}

void LuaWindow::script_selection_changed (ScriptBufferPtr sb, bool force)
{
	if (!sb) return;
	_current_buffer = sb;
	editor.set_text (sb->script);
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
}

void LuaWindow::run_script ()
{
	append_text (_("> Executing Lua script...\n"));
	auto start_time = std::chrono::high_resolution_clock::now();

	reinit_lua ();
	if (!lua) return;

	lua_State* L = lua->getState();
	std::string script_text = editor.get_text ();

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
	Gtkmm2ext::UI::instance()->flush_pending (0.05);
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

void LuaWindow::import_script () {}
void LuaWindow::save_script () {}
void LuaWindow::delete_script () {}
void LuaWindow::revert_script () {}
void LuaWindow::set_session (ARDOUR::Session* s) { SessionHandlePtr::set_session(s); update_inspector_values(); }
void LuaWindow::session_going_away () { SessionHandlePtr::session_going_away(); update_inspector_values(); }
void LuaWindow::edit_script (const std::string&, const std::string&) {}
void LuaWindow::update_title () {}
void LuaWindow::refresh_scriptlist () {}
void LuaWindow::rebuild_menu () {}
uint32_t LuaWindow::count_scratch_buffers () const { return 1; }
void LuaWindow::new_script () {}
void LuaWindow::update_gui_state () {}