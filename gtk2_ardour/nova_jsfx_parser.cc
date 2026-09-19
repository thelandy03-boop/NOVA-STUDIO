#include "nova_jsfx_parser.h"
#include <sstream>
#include <algorithm>
#include <cctype>

bool
NovaJSFXParser::is_jsfx_code(const std::string& code)
{
	return (code.find("desc:") != std::string::npos ||
	        code.find("@sample") != std::string::npos ||
	        code.find("@init") != std::string::npos ||
	        code.find("slider1") != std::string::npos);
}

static std::string trim(const std::string& str) {
	size_t first = str.find_first_not_of(" \t\r\n");
	if (std::string::npos == first) return "";
	size_t last = str.find_last_not_of(" \t\r\n");
	return str.substr(first, (last - first + 1));
}

// Reemplazo seguro que avanza el puntero y evita bucles infinitos
static void replace_all_safe(std::string& str, const std::string& from, const std::string& to) {
	if (from.empty()) return;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // Avanzar más allá del reemplazo
	}
}

std::string
NovaJSFXParser::jsfx_to_lua_dsp(const std::string& jsfx_code)
{
	std::string name = "JSFX Plugin [NOVA Engine]";
	std::string author = "JSFX Port";
	
	struct SliderParam {
		int index;
		std::string label;
		double def_val;
		double min_val;
		double max_val;
	};
	std::vector<SliderParam> sliders;

	std::string init_code = "";
	std::string slider_code = "";
	std::string sample_code = "";

	std::istringstream stream(jsfx_code);
	std::string line;
	std::string current_section = "header";

	while (std::getline(stream, line)) {
		std::string clean = trim(line);
		if (clean.empty() || clean.rfind("//", 0) == 0) continue;

		if (clean.rfind("desc:", 0) == 0) {
			name = trim(clean.substr(5));
			continue;
		}
		if (clean.rfind("//author:", 0) == 0) {
			author = trim(clean.substr(9));
			continue;
		}

		// Parse Sliders: slider1:0<-20,20>Width Boost (dB)
		if (clean.rfind("slider", 0) == 0 && current_section == "header") {
			size_t colon = clean.find(':');
			size_t lt = clean.find('<');
			size_t gt = clean.find('>');
			if (colon != std::string::npos && lt != std::string::npos && gt != std::string::npos) {
				try {
					int s_num = std::stoi(clean.substr(6, colon - 6));
					double def_v = std::stod(clean.substr(colon + 1, lt - colon - 1));
					size_t comma = clean.find(',', lt);
					double min_v = std::stod(clean.substr(lt + 1, comma - lt - 1));
					double max_v = std::stod(clean.substr(comma + 1, gt - comma - 1));
					std::string lbl = trim(clean.substr(gt + 1));

					sliders.push_back({s_num, lbl, def_v, min_v, max_v});
				} catch (...) {}
			}
			continue;
		}

		if (clean == "@init") { current_section = "init"; continue; }
		if (clean == "@slider") { current_section = "slider"; continue; }
		if (clean == "@block") { current_section = "block"; continue; }
		if (clean == "@sample") { current_section = "sample"; continue; }

		// Apilar código de las secciones EEL2
		if (current_section == "init") init_code += "    " + clean + "\n";
		if (current_section == "slider") slider_code += "    " + clean + "\n";
		if (current_section == "sample") sample_code += "        " + clean + "\n";
	}

	// Traductor EEL2 -> Lua (Sin bucles infinitos)
	auto convert_eel2_to_lua = [](std::string code) {
		replace_all_safe(code, "sqr(", "math.sqr(");
		replace_all_safe(code, "sqrt(", "math.sqrt(");
		replace_all_safe(code, "sin(", "math.sin(");
		replace_all_safe(code, "cos(", "math.cos(");
		replace_all_safe(code, "atan(", "math.atan(");
		replace_all_safe(code, "exp(", "math.exp(");
		replace_all_safe(code, "sign(", "math.sign(");
		replace_all_safe(code, "abs(", "math.abs(");

		// Reemplazar comentarios de C++ // por los de Lua --
		replace_all_safe(code, "//", "--");

		// Eliminar punto y coma final de sentencias
		std::replace(code.begin(), code.end(), ';', ' ');
		return code;
	};

	init_code = convert_eel2_to_lua(init_code);
	slider_code = convert_eel2_to_lua(slider_code);
	sample_code = convert_eel2_to_lua(sample_code);

	// Generar Código Lua DSP Oficial de Ardour / NOVA
	std::ostringstream out;
	out << "ardour {\n";
	out << "    [\"type\"] = \"dsp\",\n";
	out << "    name = \"" << name << "\",\n";
	out << "    author = \"" << author << "\",\n";
	out << "    sink_set = { [1] = { audio = 2 } },\n";
	out << "    source_set = { [1] = { audio = 2 } },\n";
	out << "    parameters = {\n";
	for (size_t i = 0; i < sliders.size(); ++i) {
		out << "        { name = \"" << sliders[i].label << "\", default = " << sliders[i].def_val
		    << ", min = " << sliders[i].min_val << ", max = " << sliders[i].max_val << " }"
		    << (i == sliders.size() - 1 ? "" : ",") << "\n";
	}
	out << "    }\n";
	out << "}\n\n";

	out << "function math.sign(x) return x >= 0 and 1 or -1 end\n";
	out << "function math.sqr(x) return x * x end\n\n";

	out << "function dsp_init (rate)\n";
	out << init_code;
	out << "end\n\n";

	out << "function dsp_run (ins, outs, n_samples)\n";
	out << "    local ctrl = ctrl or {}\n";
	for (size_t i = 0; i < sliders.size(); ++i) {
		out << "    local slider" << sliders[i].index << " = ctrl[" << (i + 1) << "] or " << sliders[i].def_val << "\n";
	}
	out << slider_code << "\n";

	out << "    local in_l, in_r = ins[1]:data(1), ins[1]:data(2)\n";
	out << "    local out_l, out_r = outs[1]:data(1), outs[1]:data(2)\n\n";

	out << "    for s = 1, n_samples do\n";
	out << "        local spl0, spl1 = in_l[s], in_r[s]\n";
	out << sample_code;
	out << "        out_l[s], out_r[s] = spl0, spl1\n";
	out << "    end\n";
	out << "end\n";

	return out.str();
}