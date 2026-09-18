#include <Scintilla.h>
#include "nova_lua_linter.h"
#include "nova_script_editor.h"
#include "lua/luastate.h"
#include <glibmm/main.h>
#include <algorithm>

NovaLuaLinter::NovaLuaLinter(NovaScriptEditor& editor)
	: _editor(editor)
	, _lua_state(nullptr)
{}

NovaLuaLinter::~NovaLuaLinter()
{
	if (_lint_connection.connected()) {
		_lint_connection.disconnect();
	}
}

void
NovaLuaLinter::set_lua_state(lua_State* L)
{
	_lua_state = L;
}

void
NovaLuaLinter::schedule_lint()
{
	if (_lint_connection.connected()) {
		_lint_connection.disconnect();
	}
	_lint_connection = Glib::signal_timeout().connect(
		sigc::mem_fun(*this, &NovaLuaLinter::run_lint), 300);
}

void
NovaLuaLinter::clear_indicators()
{
	int len = (int)_editor.send_message(SCI_GETLENGTH);
	_editor.send_message(SCI_SETINDICATORCURRENT, 0);
	_editor.send_message(SCI_INDICATORCLEARRANGE, 0, len);
}

bool
NovaLuaLinter::run_lint()
{
	_lint_connection.disconnect();
	if (!_lua_state) return false;

	clear_indicators();

	std::string code = _editor.get_text();
	if (code.empty()) {
		_editor.signal_lint_status().emit(true, "✔ Syntax OK", 0);
		return false;
	}

	int err = luaL_loadbuffer(_lua_state, code.c_str(), code.size(), "@nova_editor");
	if (err == 0) {
		lua_pop(_lua_state, 1);
		_editor.signal_lint_status().emit(true, "✔ Syntax OK", 0);
		return false;
	}

	std::string msg = lua_tostring(_lua_state, -1);
	lua_pop(_lua_state, 1);

	int err_line = 1;
	std::string err_msg = msg;
	size_t colon1 = msg.find(':');
	if (colon1 != std::string::npos) {
		size_t colon2 = msg.find(':', colon1 + 1);
		if (colon2 != std::string::npos) {
			std::string num_str = msg.substr(colon1 + 1, colon2 - colon1 - 1);
			try {
				err_line = std::stoi(num_str);
			} catch (...) {}
			err_msg = msg.substr(colon2 + 2);
		}
	}

	std::string safe_msg;
	for (char c : err_msg) {
		if (c == '<') safe_msg += "&lt;";
		else if (c == '>') safe_msg += "&gt;";
		else if (c == '&') safe_msg += "&amp;";
		else safe_msg += c;
	}

	int line = std::max(0, err_line - 1);
	int line_start = (int)_editor.send_message(SCI_POSITIONFROMLINE, line);
	int line_end   = (int)_editor.send_message(SCI_GETLINEENDPOSITION, line);

	_editor.send_message(SCI_SETINDICATORCURRENT, 0);
	_editor.send_message(SCI_INDICATORFILLRANGE, line_start, line_end - line_start);

	char buf[512];
	snprintf(buf, sizeof(buf), "✖ Line %d: %s", err_line, safe_msg.c_str());
	_editor.signal_lint_status().emit(false, buf, err_line);
	return false;
}