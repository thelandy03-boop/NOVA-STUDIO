#include "nova_reaper_compat.h"
#include "lua/luastate.h"

#include <glibmm/fileutils.h>
#include <iostream>
#include <string>
#include <vector>

void
NovaReaperCompat::inject(lua_State* L)
{
	if (!L) return;

	std::vector<std::string> paths;
	paths.push_back("gtk2_ardour/reaper_compat.lua");
	paths.push_back("../gtk2_ardour/reaper_compat.lua");
	paths.push_back("share/scripts/reaper_compat.lua");

	std::string lua_code;
	for (const auto& p : paths) {
		if (Glib::file_test(p, Glib::FILE_TEST_EXISTS)) {
			try {
				lua_code = Glib::file_get_contents(p);
				if (!lua_code.empty()) break;
			} catch (...) {}
		}
	}

	if (!lua_code.empty()) {
		luaL_dostring(L, lua_code.c_str());
	}
}