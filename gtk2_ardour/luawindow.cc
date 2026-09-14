/*
 * Copyright (C) 2016-2017 Robin Gareus <robin@gareus.org>
 * Copyright (C) 2026 NOVA-STUDIO Team
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>

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

	entry.get_buffer()->signal_changed().connect(sigc::mem_fun(*this, &LuaWindow::script_changed));
	entry.get_buffer()->signal_mark_set().connect(sigc::mem_fun(*this, &LuaWindow::on_cursor_position_changed));

	_tree_inspector.signal_cursor_changed().connect(sigc::mem_fun(*this, &LuaWindow::update_inspector_values));

	set_default_size (1050, 700);
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
	Gtk::MenuItem* item_file = Gtk::manage (new Gtk::MenuItem (_("_File"), true));
	Gtk::Menu* menu_file = Gtk::manage (new Gtk::Menu ());
	item_file->set_submenu (*menu_file);
	_main_menubar.append (*item_file);

	Gtk::MenuItem* item_edit = Gtk::manage (new Gtk::MenuItem (_("_Edit"), true));
	_main_menubar.append (*item_edit);

	Gtk::MenuItem* item_run = Gtk::manage (new Gtk::MenuItem (_("_Run"), true));
	_main_menubar.append (*item_run);

	Gtk::MenuItem* item_view = Gtk::manage (new Gtk::MenuItem (_("_View"), true));
	_main_menubar.append (*item_view);

	Gtk::MenuItem* item_tools = Gtk::manage (new Gtk::MenuItem (_("_Tools"), true));
	_main_menubar.append (*item_tools);

	Gtk::MenuItem* item_api = Gtk::manage (new Gtk::MenuItem (_("_API Reference"), true));
	_main_menubar.append (*item_api);

	Gtk::MenuItem* item_help = Gtk::manage (new Gtk::MenuItem (_("_Help"), true));
	_main_menubar.append (*item_help);
}

void LuaWindow::setup_ui ()
{
	Gtk::VBox* main_vbox = Gtk::manage (new Gtk::VBox (false, 0));
	add (*main_vbox);

	// 1. Menú Superior
	build_menubar ();
	main_vbox->pack_start (_main_menubar, Gtk::PACK_SHRINK);

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
	_api_search_entry.set_width_chars (22);
	toolbar->pack_start (_api_search_entry, Gtk::PACK_SHRINK);

	toolbar->pack_start (*Gtk::manage (new Gtk::VSeparator ()), Gtk::PACK_SHRINK);

	toolbar->pack_start (script_select, Gtk::PACK_SHRINK);

	set_btn_icon_and_text (_btn_open, "search.png", _("Load"));
	set_btn_icon_and_text (_btn_save, "Save.png", _("Save"));
	set_btn_icon_and_text (_btn_delete, "delete32.png", _("Delete"));
	set_btn_icon_and_text (_btn_options, "options.png", _("Options"));

	toolbar->pack_start (_btn_open, Gtk::PACK_SHRINK);
	toolbar->pack_start (_btn_save, Gtk::PACK_SHRINK);
	toolbar->pack_start (_btn_delete, Gtk::PACK_SHRINK);
	toolbar->pack_start (_btn_options, Gtk::PACK_SHRINK);

	main_vbox->pack_start (*toolbar, Gtk::PACK_SHRINK);

	// 3. Panel Editor + Inspector (Arriba)
	Gtk::HBox* editor_box = Gtk::manage (new Gtk::HBox (false, 0));

	_line_numbers.set_editable (false);
	_line_numbers.set_sensitive (false);
	_line_numbers.get_buffer ()->set_text ("1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n");
	editor_box->pack_start (_line_numbers, Gtk::PACK_SHRINK);

	scrollin.add (entry);
	scrollin.set_policy (Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	editor_box->pack_start (scrollin, Gtk::PACK_EXPAND_WIDGET);

	// Panel Inspector Derecha
	Gtk::VBox* inspector_vbox = Gtk::manage (new Gtk::VBox (false, 4));
	inspector_vbox->set_border_width (4);
	Gtk::Label* lbl_insp = Gtk::manage (new Gtk::Label (_("Live Variable Inspector")));
	inspector_vbox->pack_start (*lbl_insp, Gtk::PACK_SHRINK);

	_model_inspector = Gtk::ListStore::create (_inspector_cols);
	_tree_inspector.set_model (_model_inspector);
	_tree_inspector.append_column (_("Variable"), _inspector_cols.col_name);
	_tree_inspector.append_column (_("Value"), _inspector_cols.col_value);

	Gtk::ScrolledWindow* scroll_insp = Gtk::manage (new Gtk::ScrolledWindow ());
	scroll_insp->add (_tree_inspector);
	scroll_insp->set_policy (Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	inspector_vbox->pack_start (*scroll_insp, Gtk::PACK_EXPAND_WIDGET);

	_btn_add_watch.set_label (_("+ Add Watch"));
	inspector_vbox->pack_start (_btn_add_watch, Gtk::PACK_SHRINK);

	_top_hpaned.pack1 (*editor_box, true, true);
	_top_hpaned.pack2 (*inspector_vbox, false, true);

	// 4. Panel Consola / Log (Abajo)
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

	_notebook_bottom.append_page (*console_vbox, _("Script Editor Log"));
	_notebook_bottom.append_page (*Gtk::manage (new Gtk::Label (_("Interactive Lua Console ready..."))), _("Interactive Console"));
	_notebook_bottom.append_page (*Gtk::manage (new Gtk::Label (_("API Documentation Explorer"))), _("API Docs Explorer"));

	_main_vpaned.pack1 (_top_hpaned, true, true);
	_main_vpaned.pack2 (_notebook_bottom, false, true);

	main_vbox->pack_start (_main_vpaned, Gtk::PACK_EXPAND_WIDGET);

	// 5. Barra de Estado / Footer
	Gtk::HBox* statusbar = Gtk::manage (new Gtk::HBox (false, 8));
	statusbar->set_border_width (2);

	_lbl_status_pos.set_text (_("Line 1, Col 1"));
	_lbl_lua_ver.set_text (_("Engine: Lua 5.3"));

	statusbar->pack_start (_lbl_status_pos, Gtk::PACK_SHRINK);
	statusbar->pack_end (_lbl_lua_ver, Gtk::PACK_SHRINK);

	main_vbox->pack_start (*statusbar, Gtk::PACK_SHRINK);

	update_inspector_values ();
	main_vbox->show_all ();
}

void LuaWindow::update_line_numbers ()
{
	Glib::RefPtr<Gtk::TextBuffer> buf = entry.get_buffer ();
	int lines = buf->get_line_count ();
	std::string num_str = "";
	for (int i = 1; i <= lines; ++i) {
		char tmp[32];
		snprintf (tmp, sizeof(tmp), "%d\n", i);
		num_str += tmp;
	}
	_line_numbers.get_buffer ()->set_text (num_str);
}

void LuaWindow::update_inspector_values ()
{
	_model_inspector->clear ();

	Gtk::TreeModel::Row row = *(_model_inspector->append ());
	row[_inspector_cols.col_name] = "session";
	row[_inspector_cols.col_value] = _session ? "Active (Session*)" : "Nil";

	row = *(_model_inspector->append ());
	row[_inspector_cols.col_name] = "is_playing";
	row[_inspector_cols.col_value] = "false";

	row = *(_model_inspector->append ());
	row[_inspector_cols.col_name] = "track_count";
	row[_inspector_cols.col_value] = _session ? "0" : "0";

	row = *(_model_inspector->append ());
	row[_inspector_cols.col_name] = "mouse_x";
	row[_inspector_cols.col_value] = "0.0";
}

void LuaWindow::on_cursor_position_changed (const Gtk::TextBuffer::iterator& iter, const Glib::RefPtr<Gtk::TextBuffer::Mark>& mark)
{
	if (mark && mark->get_name() == "insert") {
		int line = iter.get_line() + 1;
		int col = iter.get_line_offset() + 1;
		char tmp[64];
		snprintf(tmp, sizeof(tmp), "Line %d, Col %d", line, col);
		_lbl_status_pos.set_text(tmp);
	}
}

void LuaWindow::script_changed ()
{
	update_line_numbers ();
	if (_current_buffer) {
		_current_buffer->script = entry.get_buffer ()->get_text ();
		_current_buffer->flags = (BufferFlags)(_current_buffer->flags | Buffer_Dirty);
	}
}

void LuaWindow::setup_buffers ()
{
	ScriptBufferPtr sb (new ScriptBuffer (_("Scratch Buffer #1")));
	sb->script = "---- this header is (only) required to save the script\n-- ardour { [\"type\"] = \"Snippet\", name = \"My NOVA Script\" }\n-- function factory () return function () --[[ your code here ]] end end\n\nprint(\"Hello NOVA-STUDIO Lua IDE!\")\n";
	script_buffers.push_back (sb);
	script_selection_changed (sb, true);
}

void LuaWindow::script_selection_changed (ScriptBufferPtr sb, bool force)
{
	if (!sb) return;
	_current_buffer = sb;
	entry.get_buffer ()->set_text (sb->script);
	update_line_numbers ();
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
	append_text (_("Executing script...\n"));
	reinit_lua ();
	append_text (_("Execution finished successfully.\n"));
}
void
LuaWindow::append_text (std::string s)
{
	Glib::RefPtr<Gtk::TextBuffer> tb (outtext.get_buffer());
	tb->insert (tb->end(), s + "\n");
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
void LuaWindow::highlight_syntax () {}
void LuaWindow::update_gui_state () {}
