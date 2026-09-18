#ifndef __nova_autocomplete_h__
#define __nova_autocomplete_h__

#include <string>
#include <map>
#include <set>
#include <sigc++/connection.h>

struct lua_State;
class NovaScriptEditor;

class NovaAutocomplete {
public:
	NovaAutocomplete(NovaScriptEditor& editor);
	~NovaAutocomplete();

	void set_lua_state(lua_State* L);
	void invalidate_cache();
	void handle_char(int ch);
	void update_calltip();
	void cancel_calltip();

private:
	NovaScriptEditor& _editor;
	lua_State*       _lua_state;

	sigc::connection _autoc_connection;
	int              _pending_autoc_ch;

	bool _globals_cache_dirty;
	std::string _cached_globals_list;

	bool _calltip_active;
	int  _calltip_anchor;
	std::string _calltip_signature;

	std::map<std::string, std::string> _calltip_dict;
	std::map<std::string, std::string> _native_api_dict;
	std::string _global_word_list;

	void init_native_api_dictionary();
	void init_calltip_dictionary();

	void trigger_autocompletion(int ch);
	bool trigger_deferred();
	void show_autocompletion(int len, const std::string& list);

	std::string get_word_before_pos(int pos, bool include_dots = false);
	std::string get_members_for_class(const std::string& name);
	std::string get_all_globals();
	void collect_table_keys(lua_State* L, int index, std::set<std::string>& results, int depth = 0);

	void show_calltip_at(int pos);
	void update_calltip_highlight();
};

#endif /* __nova_autocomplete_h__ */