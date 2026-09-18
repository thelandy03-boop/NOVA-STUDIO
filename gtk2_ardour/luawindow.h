/*
 * Copyright (C) 2016-2017 Robin Gareus <robin@gareus.org>
 * Copyright (C) 2026 NOVA-STUDIO Team
 */
#pragma once

#include <ytkmm/box.h>
#include <ytkmm/scrolledwindow.h>
#include <ytkmm/label.h>
#include <ytkmm/textview.h>
#include <ytkmm/window.h>
#include <ytkmm/paned.h>
#include <ytkmm/notebook.h>
#include <ytkmm/treeview.h>
#include <ytkmm/liststore.h>
#include <ytkmm/treemodelcolumn.h>
#include <ytkmm/entry.h>
#include <ytkmm/menubar.h>
#include <ytkmm/menu.h>
#include <ytkmm/menuitem.h>
#include <ytkmm/button.h>
#include <ytkmm/eventbox.h>

#include <vector>
#include <string>
#include <memory>

#include "pbd/signals.h"
#include "pbd/stateful.h"
#include "pbd/convert.h"

#include "ardour/ardour.h"
#include "ardour/luascripting.h"
#include "ardour/session_handle.h"
#include "ardour/types.h"

#include "gtkmm2ext/visibility_tracker.h"
#include "lua/luastate.h"

#include "widgets/ardour_dropdown.h"
#include "ardour_window.h"
#include "nova_script_editor.h"
#include "nova_file_explorer.h"
#include "nova_lua_console.h"

class LuaWindow :
	public ArdourWindow,
	public PBD::ScopedConnectionList
{
public:
	static LuaWindow* instance();
	~LuaWindow();

	void edit_script (const std::string&, const std::string&);
	void set_session (ARDOUR::Session* s);

	typedef enum {
		Buffer_NOFLAG     = 0x00,
		Buffer_Valid      = 0x01,
		Buffer_HasFile    = 0x02,
		Buffer_ReadOnly   = 0x04,
		Buffer_Dirty      = 0x08,
		Buffer_Scratch    = 0x10,
	} BufferFlags;

	class ScriptBuffer {
	public:
		ScriptBuffer (const std::string&);
		ScriptBuffer (ARDOUR::LuaScriptInfoPtr);
		~ScriptBuffer ();
		bool load ();

		std::string script;
		std::string name;
		std::string path;
		BufferFlags flags;
		ARDOUR::LuaScriptInfo::ScriptType type;
	};

	typedef std::shared_ptr<ScriptBuffer> ScriptBufferPtr;
	typedef std::vector<ScriptBufferPtr> ScriptBufferList;

private:
	LuaWindow ();
	static LuaWindow* _instance;

	LuaState *lua;

	Gtk::MenuBar _main_menubar;
	Gtk::VPaned _main_vpaned;
	Gtk::HPaned _top_hpaned;
	Gtk::HPaned _editor_hpaned;

	sigc::connection _script_changed_connection;
	sigc::connection _tab_switch_connection;

	Gtk::Notebook _script_notebook;
	bool          _ignore_tab_switch;

	NovaScriptEditor _editor;
	NovaFileExplorer _explorer;
	NovaLuaConsole   _console;

	Gtk::TextView outtext;
	Gtk::ScrolledWindow scrollout;

	Gtk::Button _btn_run;
	Gtk::Button _btn_pause;
	Gtk::Button _btn_stop;
	Gtk::Button _btn_autorun;

	Gtk::Entry _api_search_entry;

	Gtk::Button _btn_open;
	Gtk::Button _btn_save;
	Gtk::Button _btn_delete;
	Gtk::Button _btn_options;
	Gtk::Button _btn_explorer;

	Gtk::Button _btn_clear;
	Gtk::Button _btn_revert;
	Gtk::Button _btn_add_watch;

	ArdourWidgets::ArdourDropdown script_select;

	bool _explorer_visible;

	void toggle_explorer ();
	void open_file_in_editor (const std::string& path, const std::string& preset_content = std::string());
	void on_file_renamed (const std::string& old_path, const std::string& new_path);

	void rebuild_tab_strip ();
	void close_tab (ScriptBufferPtr sb);
	void on_script_tab_switched (GtkNotebookPage* page, guint page_num);
	std::string tab_title_for (ScriptBufferPtr sb) const;
	int  page_index_for_buffer (ScriptBufferPtr sb) const;
	Gtk::Widget* make_tab_label (ScriptBufferPtr sb);

	struct InspectorColumns : public Gtk::TreeModel::ColumnRecord {
		InspectorColumns() { add(col_name); add(col_value); }
		Gtk::TreeModelColumn<std::string> col_name;
		Gtk::TreeModelColumn<std::string> col_value;
	};
	InspectorColumns _inspector_cols;
	Gtk::TreeView _tree_inspector;
	Glib::RefPtr<Gtk::ListStore> _model_inspector;

	Gtk::Notebook _notebook_bottom;
	Gtk::Label _lbl_status_pos;
	Gtk::Label _lbl_lua_ver;

	ScriptBufferList script_buffers;
	ScriptBufferPtr _current_buffer;

	void setup_ui ();
	void build_menubar ();
	void set_btn_icon_and_text (Gtk::Button& btn, const std::string& icon_filename, const std::string& text);

	void session_going_away ();
	void update_title ();
	void reinit_lua ();

	void setup_buffers ();
	void refresh_scriptlist ();
	void rebuild_menu ();
	uint32_t count_scratch_buffers () const;

	void script_changed ();
	void script_selection_changed (ScriptBufferPtr n, bool force = false);
	void update_gui_state ();
	void update_inspector_values ();
	void on_cursor_position_changed ();
	void on_lint_status_changed (bool ok, std::string msg, int line);

	void append_text (std::string s);
	void scroll_to_bottom ();
	void clear_output ();

	void run_script ();
	void new_script ();
	void delete_script ();
	void revert_script ();
	void import_script ();
	void save_script ();
};