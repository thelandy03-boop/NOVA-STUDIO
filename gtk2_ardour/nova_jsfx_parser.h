#ifndef __nova_jsfx_parser_h__
#define __nova_jsfx_parser_h__

#include <string>
#include <vector>

struct JSFXParam {
	int index;
	std::string label;
	double def_val;
	double min_val;
	double max_val;
};

class NovaJSFXParser {
public:
	static bool is_jsfx_code(const std::string& code);
	static std::string jsfx_to_lua_dsp(const std::string& jsfx_code);
	static std::string extract_title(const std::string& jsfx_code);
	static std::string extract_author(const std::string& jsfx_code);
	static std::vector<JSFXParam> extract_params(const std::string& jsfx_code);
};

#endif /* __nova_jsfx_parser_h__ */