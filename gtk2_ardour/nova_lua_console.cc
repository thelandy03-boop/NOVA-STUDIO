#include <ytk/ytk.h>
#include "nova_lua_console.h"
#include "lua/luastate.h"

#include <iostream>
#include <sstream>

NovaLuaConsole::NovaLuaConsole()
	: _lua_state(nullptr)
	, _history_index(-1)
{
	this->set_spacing(4);
	this->set_border_width(4);

	// Output text view con estilo Cyberpunk
	_text_output.set_editable(false);
	_text_output.set_wrap_mode(Gtk::WRAP_WORD_CHAR);

	Glib::RefPtr<Gtk::TextBuffer> buffer = _text_output.get_buffer();
	buffer->set_text("NOVA-STUDIO Interactive Lua Console v1.0\nType Lua expressions (e.g. Session:instance():name()) and press Enter.\n\n");

	_scroll_output.add(_text_output);
	_scroll_output.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

	// Input bar
	_prompt_label.set_markup("<span foreground='#00F0FF' weight='bold'>lua&gt; </span>");
	_input_entry.set_width_chars(50);

	_input_box.set_spacing(4);
	_input_box.pack_start(_prompt_label, false, false, 0);
	_input_box.pack_start(_input_entry, true, true, 0);

	this->pack_start(_scroll_output, true, true, 0);
	this->pack_start(_input_box, false, false, 0);

	_input_entry.signal_activate().connect(sigc::mem_fun(*this, &NovaLuaConsole::execute_current_input));
	g_signal_connect(_input_entry.gobj(), "key-press-event", G_CALLBACK(&NovaLuaConsole::on_input_key_press), this);

	this->show_all_children();
}

NovaLuaConsole::~NovaLuaConsole() {}

void
NovaLuaConsole::set_lua_state(lua_State* L)
{
	_lua_state = L;
}

void
NovaLuaConsole::clear_console()
{
	_text_output.get_buffer()->set_text("");
}

void
NovaLuaConsole::append_output(const std::string& text, const std::string& color_hex)
{
	Glib::RefPtr<Gtk::TextBuffer> tb = _text_output.get_buffer();
	Gtk::TextBuffer::iterator end_it = tb->end();

	if (!color_hex.empty()) {
		Glib::RefPtr<Gtk::TextBuffer::Tag> tag = tb->create_tag();
		tag->property_foreground() = color_hex;
		tb->insert_with_tag(end_it, text, tag);
	} else {
		tb->insert(end_it, text);
	}

	scroll_to_bottom();
}

void
NovaLuaConsole::scroll_to_bottom()
{
	Gtk::Adjustment* adj = _scroll_output.get_vadjustment();
	if (adj) {
		adj->set_value(adj->get_upper() - adj->get_page_size());
	}
}

std::string
NovaLuaConsole::format_lua_value(int index)
{
	if (!_lua_state) return "nil";

	int type = lua_type(_lua_state, index);
	switch (type) {
	case LUA_TNIL:
		return "nil";
	case LUA_TBOOLEAN:
		return lua_toboolean(_lua_state, index) ? "true" : "false";
	case LUA_TNUMBER:
		return std::to_string(lua_tonumber(_lua_state, index));
	case LUA_TSTRING:
		return std::string("\"") + lua_tostring(_lua_state, index) + "\"";
	default: {
		const char* name = lua_typename(_lua_state, type);
		char buf[128];
		snprintf(buf, sizeof(buf), "&lt;%s: %p&gt;", name, lua_topointer(_lua_state, index));
		return buf;
	}
	}
}

void
NovaLuaConsole::eval_lua(const std::string& code)
{
	if (code == "clear" || code == "clear()") {
		clear_console();
		return;
	}

	if (!_lua_state) {
		append_output("[Error] Lua State not initialized.\n", "#FF3366");
		return;
	}

	int top_before = lua_gettop(_lua_state);

	// Intentar evaluarlo como expresion con 'return <code_line>'
	std::string expr = "return " + code;
	int err = luaL_loadstring(_lua_state, expr.c_str());

	if (err != 0) {
		lua_pop(_lua_state, 1); // Remover mensaje de error del intento 'return'
		// Cargar como sentencia normal
		err = luaL_loadstring(_lua_state, code.c_str());
	}

	if (err != 0) {
		std::string err_msg = lua_tostring(_lua_state, -1);
		lua_pop(_lua_state, 1);
		append_output("✖ " + err_msg + "\n", "#FF3366");
		return;
	}

	// Ejecutar con Pre-Crash Guard
	err = lua_pcall(_lua_state, 0, LUA_MULTRET, 0);
	if (err != 0) {
		std::string err_msg = lua_tostring(_lua_state, -1);
		lua_pop(_lua_state, 1);
		append_output("✖ Runtime Error: " + err_msg + "\n", "#FF3366");
		return;
	}

	int top_after = lua_gettop(_lua_state);
	int nresults = top_after - top_before;

	if (nresults > 0) {
		std::string res_str = "=> ";
		for (int i = top_before + 1; i <= top_after; ++i) {
			if (i > top_before + 1) res_str += ", ";
			res_str += format_lua_value(i);
		}
		res_str += "\n";
		append_output(res_str, "#51CF66");
		lua_pop(_lua_state, nresults);
	} else {
		append_output("✔ OK\n", "#00F0FF");
	}
}

void
NovaLuaConsole::execute_current_input()
{
	std::string input = _input_entry.get_text();
	if (input.empty()) return;

	append_output("lua> " + input + "\n", "#D4D4D4");

	_history.push_back(input);
	_history_index = _history.size();

	_input_entry.set_text("");

	eval_lua(input);
}

gboolean
NovaLuaConsole::on_input_key_press(GtkWidget* /*widget*/, GdkEventKey* event, gpointer data)
{
	NovaLuaConsole* console = static_cast<NovaLuaConsole*>(data);
	if (!console || !event) return FALSE;

	if (event->keyval == 0xFF52) { // Up Arrow
		if (!console->_history.empty() && console->_history_index > 0) {
			--console->_history_index;
			console->_input_entry.set_text(console->_history[console->_history_index]);
			console->_input_entry.set_position(-1);
		}
		return TRUE;
	}
	if (event->keyval == 0xFF54) { // Down Arrow
		if (console->_history_index < (int)console->_history.size() - 1) {
			++console->_history_index;
			console->_input_entry.set_text(console->_history[console->_history_index]);
			console->_input_entry.set_position(-1);
		} else {
			console->_history_index = console->_history.size();
			console->_input_entry.set_text("");
		}
		return TRUE;
	}

	return FALSE;
}