#ifndef __nova_jsfx_parser_h__
#define __nova_jsfx_parser_h__

#include <string>
#include <vector>

class NovaJSFXParser {
public:
	static bool is_jsfx_code(const std::string& code);
	static std::string jsfx_to_lua_dsp(const std::string& jsfx_code);
};

#endif /* __nova_jsfx_parser_h__ */