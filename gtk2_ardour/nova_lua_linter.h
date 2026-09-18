#ifndef __nova_lua_linter_h__
#define __nova_lua_linter_h__

#include <string>
#include <sigc++/connection.h>

struct lua_State;
class NovaScriptEditor;

class NovaLuaLinter {
public:
	NovaLuaLinter(NovaScriptEditor& editor);
	~NovaLuaLinter();

	void set_lua_state(lua_State* L);
	void schedule_lint();
	void clear_indicators();

private:
	NovaScriptEditor& _editor;
	lua_State*       _lua_state;
	sigc::connection _lint_connection;

	bool run_lint();
};

#endif /* __nova_lua_linter_h__ */