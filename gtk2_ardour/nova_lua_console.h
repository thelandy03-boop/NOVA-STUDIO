#ifndef __nova_lua_console_h__
#define __nova_lua_console_h__

#include <ytkmm/box.h>
#include <ytkmm/entry.h>
#include <ytkmm/textview.h>
#include <ytkmm/scrolledwindow.h>
#include <ytkmm/label.h>
#include <vector>
#include <string>

struct lua_State;
struct _GtkWidget;
typedef struct _GtkWidget GtkWidget;
typedef struct _GdkEventKey GdkEventKey;

class NovaLuaConsole : public Gtk::VBox {
public:
	NovaLuaConsole();
	~NovaLuaConsole();

	void set_lua_state(lua_State* L);
	void append_output(const std::string& text, const std::string& color_hex = "");
	void clear_console();

private:
	lua_State* _lua_state;

	Gtk::ScrolledWindow _scroll_output;
	Gtk::TextView       _text_output;
	Gtk::HBox           _input_box;
	Gtk::Label          _prompt_label;
	Gtk::Entry          _input_entry;

	std::vector<std::string> _history;
	int                      _history_index;

	void execute_current_input();
	void eval_lua(const std::string& code);
	void scroll_to_bottom();
	std::string format_lua_value(int index);

	static gboolean on_input_key_press(GtkWidget* widget, GdkEventKey* event, gpointer data);
};

#endif /* __nova_lua_console_h__ */