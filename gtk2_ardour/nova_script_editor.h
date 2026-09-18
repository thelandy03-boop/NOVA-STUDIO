#ifndef __nova_script_editor_h__
#define __nova_script_editor_h__

#include <ytkmm/box.h>
#include <string>

struct lua_State;
struct _GtkWidget;
typedef struct _GtkWidget GtkWidget;

struct _ScintillaObject;
typedef struct _ScintillaObject ScintillaObject;

struct SCNotification;

// Componentes modulares forward-declared
class NovaSearchBar;
class NovaLuaLinter;
class NovaAutocomplete;

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
	void grab_editor_focus();

	intptr_t send_message(unsigned int msg, uintptr_t wp = 0, intptr_t lp = 0) const;

	sigc::signal<void>& signal_text_changed()   { return _signal_text_changed; }
	sigc::signal<void>& signal_cursor_changed() { return _signal_cursor_changed; }
	sigc::signal<void, bool, std::string, int>& signal_lint_status() { return _signal_lint_status; }

private:
	ScintillaObject*  _sci;
	bool              _setting_text;

	// Punteros a los componentes modulares
	NovaSearchBar*    _search_bar;
	NovaLuaLinter*    _linter;
	NovaAutocomplete* _autocomplete;

	sigc::signal<void> _signal_text_changed;
	sigc::signal<void> _signal_cursor_changed;
	sigc::signal<void, bool, std::string, int> _signal_lint_status;

	void set_dark_cyberpunk_theme();
	void set_lua_syntax();
	void do_smart_indent(int ch);

	static void on_notify(GtkWidget* w, gint param, SCNotification* n, gpointer data);
	static gboolean on_key_press(GtkWidget* widget, GdkEventKey* event, gpointer data);
	static gboolean on_scroll(GtkWidget* widget, GdkEventScroll* event, gpointer data);
};

#endif /* __nova_script_editor_h__ */