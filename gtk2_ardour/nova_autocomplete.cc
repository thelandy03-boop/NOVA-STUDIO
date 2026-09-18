#include <Scintilla.h>
#include "nova_autocomplete.h"
#include "nova_script_editor.h"
#include "lua/luastate.h"
#include <glibmm/main.h>
#include <sstream>
#include <algorithm>
#include <cctype>

NovaAutocomplete::NovaAutocomplete(NovaScriptEditor& editor)
	: _editor(editor)
	, _lua_state(nullptr)
	, _pending_autoc_ch(0)
	, _globals_cache_dirty(true)
	, _calltip_active(false)
	, _calltip_anchor(0)
{
	init_native_api_dictionary();
	init_calltip_dictionary();
}

NovaAutocomplete::~NovaAutocomplete()
{
	if (_autoc_connection.connected()) {
		_autoc_connection.disconnect();
	}
}

void
NovaAutocomplete::set_lua_state(lua_State* L)
{
	_lua_state = L;
	invalidate_cache();
}

void
NovaAutocomplete::invalidate_cache()
{
	_globals_cache_dirty = true;
	_cached_globals_list.clear();
}

void
NovaAutocomplete::handle_char(int ch)
{
	if (ch == '(') {
		int pos = (int)_editor.send_message(SCI_GETCURRENTPOS);
		_editor.send_message(SCI_AUTOCCANCEL);
		show_calltip_at(pos - 1);
		return;
	}
	if (ch == ',') {
		if (_calltip_active) update_calltip_highlight();
		return;
	}
	if (ch == ')' || ch == '\n' || ch == '\r') {
		cancel_calltip();
		return;
	}
	if (!_calltip_active) {
		trigger_autocompletion(ch);
	}
}

void
NovaAutocomplete::trigger_autocompletion(int ch)
{
	_pending_autoc_ch = ch;
	if (_autoc_connection.connected()) {
		_autoc_connection.disconnect();
	}
	_autoc_connection = Glib::signal_timeout().connect(
		sigc::mem_fun(*this, &NovaAutocomplete::trigger_deferred), 80);
}

bool
NovaAutocomplete::trigger_deferred()
{
	_autoc_connection.disconnect();
	int pos = (int)_editor.send_message(SCI_GETCURRENTPOS);

	if (_pending_autoc_ch == ':' || _pending_autoc_ch == '.') {
		std::string obj_name = get_word_before_pos(pos - 1, false);
		if (!obj_name.empty()) {
			std::string members = get_members_for_class(obj_name);
			if (!members.empty()) {
				show_autocompletion(0, members);
				return false;
			}
		}
	} else if (std::isalnum(_pending_autoc_ch) || _pending_autoc_ch == '_') {
		std::string word = get_word_before_pos(pos, false);
		if (word.length() >= 2) {
			show_autocompletion((int)word.length(), get_all_globals());
			return false;
		}
	}
	return false;
}

void
NovaAutocomplete::show_autocompletion(int len, const std::string& list)
{
	if (list.empty()) {
		_editor.send_message(SCI_AUTOCCANCEL);
		return;
	}
	_editor.send_message(SCI_AUTOCSHOW, len, (intptr_t)list.c_str());
}

std::string
NovaAutocomplete::get_word_before_pos(int pos, bool include_dots)
{
	if (pos <= 0) return "";
	int start = pos;
	while (start > 0) {
		int ch = (int)_editor.send_message(SCI_GETCHARAT, start - 1);
		bool valid = std::isalnum(ch) || ch == '_';
		if (include_dots && (ch == '.' || ch == ':')) valid = true;
		if (!valid) break;
		--start;
	}
	if (start >= pos) return "";
	std::string word;
	for (int i = start; i < pos; ++i) {
		word += (char)_editor.send_message(SCI_GETCHARAT, i);
	}
	return word;
}

std::string
NovaAutocomplete::get_members_for_class(const std::string& class_name)
{
	auto it = _native_api_dict.find(class_name);
	if (it != _native_api_dict.end()) return it->second;

	if (_lua_state) {
		std::set<std::string> keys;
		lua_getglobal(_lua_state, class_name.c_str());
		if (lua_istable(_lua_state, -1)) {
			collect_table_keys(_lua_state, -1, keys, 0);
		}
		lua_pop(_lua_state, 1);

		if (!keys.empty()) {
			std::string list;
			for (auto const& k : keys) {
				if (!list.empty()) list += " ";
				list += k;
			}
			return list;
		}
	}
	return "";
}

void
NovaAutocomplete::collect_table_keys(lua_State* L, int index, std::set<std::string>& results, int depth)
{
	if (depth > 2) return;
	int abs_index = lua_absindex(L, index);
	lua_pushnil(L);
	while (lua_next(L, abs_index) != 0) {
		if (lua_type(L, -2) == LUA_TSTRING) {
			results.insert(lua_tostring(L, -2));
		}
		lua_pop(L, 1);
	}
	if (lua_getmetatable(L, abs_index)) {
		lua_pushstring(L, "__index");
		lua_rawget(L, -2);
		if (lua_istable(L, -1)) {
			collect_table_keys(L, -1, results, depth + 1);
		}
		lua_pop(L, 2);
	}
}

std::string
NovaAutocomplete::get_all_globals()
{
	if (!_globals_cache_dirty && !_cached_globals_list.empty()) {
		return _cached_globals_list;
	}
	std::set<std::string> all_words;
	std::istringstream iss(_global_word_list);
	std::string item;
	while (iss >> item) all_words.insert(item);

	if (_lua_state) {
		lua_pushglobaltable(_lua_state);
		collect_table_keys(_lua_state, -1, all_words, 0);
		lua_pop(_lua_state, 1);
	}
	std::string merged;
	for (auto const& w : all_words) {
		if (!merged.empty()) merged += " ";
		merged += w;
	}
	_cached_globals_list = merged;
	_globals_cache_dirty = false;
	return _cached_globals_list;
}

void
NovaAutocomplete::show_calltip_at(int pos_paren)
{
	std::string func_name = get_word_before_pos(pos_paren, true);
	if (func_name.empty()) return;

	auto it = _calltip_dict.find(func_name);
	if (it == _calltip_dict.end()) return;

	_calltip_signature = it->second;
	_calltip_anchor    = pos_paren;
	_calltip_active    = true;

	_editor.send_message(SCI_CALLTIPSHOW, pos_paren, (intptr_t)_calltip_signature.c_str());
	_editor.send_message(SCI_CALLTIPSETFORE, 0xD4D4D4);
	_editor.send_message(SCI_CALLTIPSETBACK, 0x2A2D2E);
	_editor.send_message(SCI_CALLTIPSETFOREHLT, 0x00F0FF);

	update_calltip_highlight();
}

void
NovaAutocomplete::cancel_calltip()
{
	if (!_calltip_active) return;
	_editor.send_message(SCI_CALLTIPCANCEL);
	_calltip_active = false;
	_calltip_anchor = 0;
	_calltip_signature.clear();
}

void
NovaAutocomplete::update_calltip()
{
	if (!_calltip_active) return;
	int cur_pos  = (int)_editor.send_message(SCI_GETCURRENTPOS);
	int cur_line = (int)_editor.send_message(SCI_LINEFROMPOSITION, cur_pos);
	int anc_line = (int)_editor.send_message(SCI_LINEFROMPOSITION, _calltip_anchor);

	if (cur_line != anc_line || cur_pos <= _calltip_anchor) {
		cancel_calltip();
	} else {
		update_calltip_highlight();
	}
}

void
NovaAutocomplete::update_calltip_highlight()
{
	if (!_calltip_active) return;
	int cur_pos = (int)_editor.send_message(SCI_GETCURRENTPOS);
	if (cur_pos <= _calltip_anchor) {
		cancel_calltip();
		return;
	}

	int comma_count = 0, paren_depth = 0;
	for (int i = _calltip_anchor + 1; i < cur_pos; ++i) {
		int ch = (int)_editor.send_message(SCI_GETCHARAT, i);
		if (ch == '(') ++paren_depth;
		else if (ch == ')') {
			if (paren_depth == 0) { cancel_calltip(); return; }
			--paren_depth;
		} else if (ch == ',' && paren_depth == 0) {
			++comma_count;
		}
	}

	size_t open_p  = _calltip_signature.find('(');
	size_t close_p = _calltip_signature.rfind(')');
	if (open_p == std::string::npos || close_p == std::string::npos || close_p <= open_p + 1) {
		_editor.send_message(SCI_CALLTIPSETHLT, 0, 0);
		return;
	}

	std::string params = _calltip_signature.substr(open_p + 1, close_p - open_p - 1);
	std::vector<std::pair<int, int> > param_ranges;
	int pstart = 0, depth  = 0;
	for (int i = 0; i < (int)params.size(); ++i) {
		char c = params[i];
		if (c == '(' || c == '[') ++depth;
		else if (c == ')' || c == ']') --depth;
		else if (c == ',' && depth == 0) {
			param_ranges.push_back(std::make_pair(pstart, i));
			pstart = i + 1;
		}
	}
	param_ranges.push_back(std::make_pair(pstart, (int)params.size()));

	int active = std::min(comma_count, (int)param_ranges.size() - 1);
	if (active < 0) active = 0;

	int hl_start = (int)open_p + 1 + param_ranges[active].first;
	int hl_end   = (int)open_p + 1 + param_ranges[active].second;

	while (hl_start < hl_end && _calltip_signature[hl_start] == ' ') ++hl_start;
	while (hl_end > hl_start && _calltip_signature[hl_end - 1] == ' ') --hl_end;

	_editor.send_message(SCI_CALLTIPSETHLT, hl_start, hl_end);
}

void
NovaAutocomplete::init_native_api_dictionary()
{
	_native_api_dict["Session"] = "config get_tracks instance sample_rate tempo_map transport_locate transport_rolling transport_sample transport_start transport_stop";
	_native_api_dict["Editor"] = "get_selection instance";
	_native_api_dict["AudioTrack"] = "gain mute n_channels name solo";
	_native_api_dict["AudioBuffer"] = "apply_gain clear data read_from silence size write_to";
	_native_api_dict["MidiBuffer"] = "clear events size";
	_native_api_dict["TempoMap"] = "meter_at tempo_at";
	_native_api_dict["Plugin"] = "name parameter set_parameter";
	_native_api_dict["math"] = "abs acos asin atan ceil cos exp floor huge log max min pi random randomseed sin sqrt tan tanh";
	_native_api_dict["string"] = "byte char find format gsub len lower match rep reverse sub upper";
	_native_api_dict["table"] = "concat insert remove sort unpack";
	_native_api_dict["os"] = "clock date time";
	_native_api_dict["io"] = "close open read write";

	_global_word_list = "AudioBuffer AudioTrack Editor MidiBuffer Plugin Session TempoMap assert error ipairs math next os pairs pcall print rawget rawset require select string table tonumber tostring type unpack xpcall";
}

void
NovaAutocomplete::init_calltip_dictionary()
{
	_calltip_dict = {
		{"Session:instance", "Session:instance() -> Session"},
		{"Session:transport_rolling", "Session:transport_rolling() -> bool"},
		{"Session:transport_sample", "Session:transport_sample() -> int64"},
		{"Session:sample_rate", "Session:sample_rate() -> int"},
		{"Session:get_tracks", "Session:get_tracks() -> TrackList"},
		{"Session:transport_locate", "Session:transport_locate(sample: int64, roll: bool)"},
		{"Session:transport_start", "Session:transport_start()"},
		{"Session:transport_stop", "Session:transport_stop()"},
		{"Session:tempo_map", "Session:tempo_map() -> TempoMap"},
		{"Session:config", "Session:config() -> Config"},
		{"Editor:instance", "Editor:instance() -> Editor"},
		{"Editor:get_selection", "Editor:get_selection() -> Selection"},
		{"AudioTrack:name", "AudioTrack:name() -> string"},
		{"AudioTrack:n_channels", "AudioTrack:n_channels() -> int"},
		{"AudioTrack:gain", "AudioTrack:gain() -> float"},
		{"AudioTrack:mute", "AudioTrack:mute() -> bool"},
		{"AudioTrack:solo", "AudioTrack:solo() -> bool"},
		{"AudioBuffer:data", "AudioBuffer:data() -> float*"},
		{"AudioBuffer:size", "AudioBuffer:size() -> int"},
		{"AudioBuffer:silence", "AudioBuffer:silence(samples: int, offset: int)"},
		{"AudioBuffer:read_from", "AudioBuffer:read_from(src: AudioBuffer, samples: int, src_offset: int)"},
		{"AudioBuffer:write_to", "AudioBuffer:write_to(dst: AudioBuffer, samples: int, dst_offset: int)"},
		{"AudioBuffer:apply_gain", "AudioBuffer:apply_gain(gain: float, samples: int, offset: int)"},
		{"AudioBuffer:clear", "AudioBuffer:clear()"},
		{"MidiBuffer:events", "MidiBuffer:events() -> MidiEventList"},
		{"MidiBuffer:size", "MidiBuffer:size() -> int"},
		{"MidiBuffer:clear", "MidiBuffer:clear()"},
		{"TempoMap:tempo_at", "TempoMap:tempo_at(sample: int64) -> double"},
		{"TempoMap:meter_at", "TempoMap:meter_at(sample: int64) -> Meter"},
		{"Plugin:name", "Plugin:name() -> string"},
		{"Plugin:parameter", "Plugin:parameter(index: int) -> float"},
		{"Plugin:set_parameter", "Plugin:set_parameter(index: int, value: float)"},
		{"math.sin", "math.sin(x: number) -> number"},
		{"math.cos", "math.cos(x: number) -> number"},
		{"math.tan", "math.tan(x: number) -> number"},
		{"math.asin", "math.asin(x: number) -> number"},
		{"math.acos", "math.acos(x: number) -> number"},
		{"math.atan", "math.atan(y: number, x: number?) -> number"},
		{"math.abs", "math.abs(x: number) -> number"},
		{"math.floor", "math.floor(x: number) -> int"},
		{"math.ceil", "math.ceil(x: number) -> int"},
		{"math.sqrt", "math.sqrt(x: number) -> number"},
		{"math.log", "math.log(x: number, base: number?) -> number"},
		{"math.exp", "math.exp(x: number) -> number"},
		{"math.min", "math.min(a: number, b: number, ...) -> number"},
		{"math.max", "math.max(a: number, b: number, ...) -> number"},
		{"math.random", "math.random(m: int?, n: int?) -> number"},
		{"math.randomseed", "math.randomseed(seed: int)"},
		{"math.tanh", "math.tanh(x: number) -> number"},
		{"string.format", "string.format(fmt: string, ...) -> string"},
		{"string.find", "string.find(s: string, pattern: string, init: int?) -> int, int"},
		{"string.sub", "string.sub(s: string, i: int, j: int?) -> string"},
		{"string.len", "string.len(s: string) -> int"},
		{"string.match", "string.match(s: string, pattern: string, init: int?) -> string"},
		{"string.gsub", "string.gsub(s: string, pattern: string, repl: string, n: int?) -> string, int"},
		{"string.byte", "string.byte(s: string, i: int?, j: int?) -> int..."},
		{"string.char", "string.char(...) -> string"},
		{"string.rep", "string.rep(s: string, n: int, sep: string?) -> string"},
		{"string.reverse", "string.reverse(s: string) -> string"},
		{"string.lower", "string.lower(s: string) -> string"},
		{"string.upper", "string.upper(s: string) -> string"},
		{"table.insert", "table.insert(list: table, pos: int?, value: any)"},
		{"table.remove", "table.remove(list: table, pos: int?) -> any"},
		{"table.sort", "table.sort(list: table, comp: function?)"},
		{"table.concat", "table.concat(list: table, sep: string?, i: int?, j: int?) -> string"},
		{"table.unpack", "table.unpack(list: table, i: int?, j: int?) -> ..."},
		{"os.clock", "os.clock() -> number"},
		{"os.time", "os.time(table: table?) -> int"},
		{"os.date", "os.date(fmt: string?, time: int?) -> string|table"},
		{"io.open", "io.open(filename: string, mode: string?) -> file?"},
		{"io.read", "io.read(...) -> ..."},
		{"io.write", "io.write(...) -> file"},
		{"io.close", "io.close(file: file?)"},
		{"print", "print(...)"},
		{"type", "type(v: any) -> string"},
		{"tostring", "tostring(v: any) -> string"},
		{"tonumber", "tonumber(e: any, base: int?) -> number?"},
		{"pairs", "pairs(t: table) -> iterator"},
		{"ipairs", "ipairs(t: table) -> iterator"},
		{"pcall", "pcall(f: function, ...) -> bool, ..."},
		{"xpcall", "xpcall(f: function, msgh: function, ...) -> bool, ..."},
		{"require", "require(modname: string) -> any"},
		{"error", "error(message: string, level: int?)"},
		{"assert", "assert(v: any, message: string?) -> any"}
	};
}