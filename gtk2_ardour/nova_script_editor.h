#ifndef __nova_script_editor_h__
#define __nova_script_editor_h__

#include <ytkmm/box.h>
#include <ytkmm/entry.h>
#include <ytkmm/label.h>
#include <ytkmm/button.h>
#include <ytkmm/frame.h>

#include <string>
#include <map>
#include <set>
#include <vector>
#include <cstdint>

struct lua_State;
struct _GtkWidget;
typedef struct _GtkWidget GtkWidget;

struct _ScintillaObject;
typedef struct _ScintillaObject ScintillaObject;

struct SCNotification;

class NovaScriptEditor : public Gtk::VBox {
public:
	NovaScriptEditor();
	virtual ~NovaScriptEditor();

	void set_lua_state(lua_State* L);
	void invalidate_globals_cache();

	void set_text(const std::string& text);
	std::string get_text() const;

	void get_cursor_position(int& line, int& col) const;

	void show_search_bar(bool focus_replace = false);
	void hide_search_bar();

	sigc::signal<void>& signal_text_changed()   { return _signal_text_changed; }
	sigc::signal<void>& signal_cursor_changed() { return _signal_cursor_changed; }
	sigc::signal<void, bool, std::string, int>& signal_lint_status() { return _signal_lint_status; }

private:
	ScintillaObject* _sci;
	lua_State*       _lua_state;

	Gtk::HBox   _top_overlay_bar;
	Gtk::Frame  _search_frame;
	Gtk::VBox   _search_panel;
	Gtk::HBox   _find_row;
	Gtk::HBox   _replace_row;

	Gtk::Button _btn_toggle_replace;
	Gtk::Entry  _search_entry;
	Gtk::Label  _lbl_match_count;
	Gtk::Button _btn_find_prev;
	Gtk::Button _btn_find_next;
	Gtk::Button _btn_close_search;

	Gtk::Entry  _replace_entry;
	Gtk::Button _btn_replace;
	Gtk::Button _btn_replace_all;

	bool _replace_visible;
	int  _match_current;
	int  _match_total;

	bool _setting_text;

	sigc::signal<void> _signal_text_changed;
	sigc::signal<void> _signal_cursor_changed;
	sigc::signal<void, bool, std::string, int> _signal_lint_status;

	sigc::connection _lint_timeout_connection;
	sigc::connection _autoc_timeout_connection;

	int  _pending_autoc_ch;
	bool _globals_cache_dirty;
	std::string _cached_globals_list;

	bool _calltip_active;
	int  _calltip_anchor;
	std::string _calltip_signature;
	std::map<std::string, std::string> _calltip_dict;

	std::map<std::string, std::string> _native_api_dict;
	std::string _global_word_list;

	intptr_t send_sci(unsigned int msg, uintptr_t wp = 0, intptr_t lp = 0) const;

	void set_dark_cyberpunk_theme();
	void set_lua_syntax();
	void setup_autocompletion();
	void init_native_api_dictionary();
	void init_calltip_dictionary();

	void toggle_replace_row();
	void do_find_next();
	void do_find_prev();
	void do_replace();
	void do_replace_all();
	void on_search_text_changed();
	void update_match_count();
	int  count_all_matches(const std::string& term);
	bool find_with_dir(bool forward);

	void schedule_autocompletion(int ch);
	bool trigger_autocompletion_deferred();
	void show_autocompletion(int len_entered, const std::string& word_list);
	std::string get_word_before_pos(int pos, bool include_dots = false);
	std::string get_members_for_class(const std::string& class_name);
	std::string get_all_globals();
	void collect_table_keys(lua_State* L, int index, std::set<std::string>& results, int depth = 0);

	void do_smart_indent(int ch);
	void handle_char_added(int ch);

	void show_calltip(int pos_paren);
	void cancel_calltip();
	void update_calltip_highlight();

	void schedule_lint();
	bool run_lint();
	void clear_lint_indicators();

	static void on_notify(GtkWidget* w, gint param, SCNotification* n, gpointer data);
	static gboolean on_scintilla_key_press(GtkWidget* widget, GdkEventKey* event, gpointer data);
	static gboolean on_scintilla_scroll(GtkWidget* widget, GdkEventScroll* event, gpointer data);
	static gboolean on_search_entry_key(GtkWidget* widget, GdkEventKey* event, gpointer data);
};

#endif /* __nova_script_editor_h__ */