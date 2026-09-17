#ifndef GTK
#define GTK
#endif

#include "nova_script_editor.h"

NovaScriptEditor::NovaScriptEditor()
	: Gtk::VBox(false, 0)
	, _sci(nullptr)
{
	// 1. Crear el widget Scintilla
	GtkWidget* w = scintilla_object_new();
	_sci = SCINTILLA_OBJECT(w);

	// 2. Envolver y empaquetar SIN forzar realize/show prematuro
	Gtk::Widget* sci_widget = Gtk::manage(Glib::wrap(w));
	pack_start(*sci_widget, Gtk::PACK_EXPAND_WIDGET);

	// 3. Configurar sintaxis y tema (mensajes Scintilla funcionan en memoria)
	set_lua_syntax();
	set_dark_cyberpunk_theme();
	setup_autocompletion();

	// 4. Conectar señales
	g_signal_connect(G_OBJECT(w), "sci-notify", G_CALLBACK(on_notify), this);
}

NovaScriptEditor::~NovaScriptEditor()
{
}

intptr_t NovaScriptEditor::send_sci(unsigned int msg, uintptr_t wp, intptr_t lp) const
{
	if (!_sci) return 0;
	return scintilla_object_send_message(_sci, msg, wp, lp);
}

void NovaScriptEditor::set_text(const std::string& text)
{
	send_sci(SCI_SETTEXT, 0, (intptr_t)text.c_str());
}

std::string NovaScriptEditor::get_text() const
{
	int len = send_sci(SCI_GETLENGTH, 0, 0);
	if (len <= 0) return "";

	std::string buf(len + 1, '\0');
	send_sci(SCI_GETTEXT, len + 1, (intptr_t)&buf[0]);
	buf.resize(len);
	return buf;
}

void NovaScriptEditor::set_lua_syntax()
{
	send_sci(SCI_SETLEXER, SCLEX_LUA, 0);

	const char* lua_keywords = 
		"and break do else elseif end false for function if in local "
		"nil not or repeat return then true until while "
		"Session Editor AudioTrack MidiTrack TempoMap";

	send_sci(SCI_SETKEYWORDS, 0, (intptr_t)lua_keywords);

	// Plegado de código (Code Folding)
	send_sci(SCI_SETPROPERTY, (uintptr_t)"fold", (intptr_t)"1");
	send_sci(SCI_SETPROPERTY, (uintptr_t)"fold.compact", (intptr_t)"0");

	// Margen 2 configurado para iconos de plegado
	send_sci(SCI_SETMARGINTYPEN, 2, SC_MARGIN_SYMBOL);
	send_sci(SCI_SETMARGINWIDTHN, 2, 16);
	send_sci(SCI_SETMARGINMASKN, 2, SC_MASK_FOLDERS);
	send_sci(SCI_SETMARGINSENSITIVEN, 2, 1);
}

void NovaScriptEditor::set_dark_cyberpunk_theme()
{
	// Fuente y estilo base
	send_sci(SCI_STYLESETFONT, STYLE_DEFAULT, (intptr_t)"Monospace");
	send_sci(SCI_STYLESETSIZE, STYLE_DEFAULT, 10);
	send_sci(SCI_STYLESETFORE, STYLE_DEFAULT, 0xD4D4D4);
	send_sci(SCI_STYLESETBACK, STYLE_DEFAULT, 0x1E1E1E);
	send_sci(SCI_STYLECLEARALL, 0, 0);

	// Colores de Sintaxis Lua (Cyberpunk Dark)
	send_sci(SCI_STYLESETFORE, SCE_LUA_WORD, 0xDCDCAA);
	send_sci(SCI_STYLESETBOLD, SCE_LUA_WORD, 1);
	send_sci(SCI_STYLESETFORE, SCE_LUA_COMMENT, 0x6A9955);
	send_sci(SCI_STYLESETFORE, SCE_LUA_COMMENTLINE, 0x6A9955);
	send_sci(SCI_STYLESETFORE, SCE_LUA_STRING, 0xCE9178);
	send_sci(SCI_STYLESETFORE, SCE_LUA_NUMBER, 0xB5CEA8);
	send_sci(SCI_STYLESETFORE, SCE_LUA_OPERATOR, 0xD4D4D4);

	// Caret / Cursor Neón Cyan
	send_sci(SCI_SETCARETLINEVISIBLE, 1, 0);
	send_sci(SCI_SETCARETLINEBACK, 0x2A2D2E, 0);
	send_sci(SCI_SETCARETFORE, 0x00F0FF, 0);

	// Margen 0: Números de Línea
	send_sci(SCI_SETMARGINTYPEN, 0, SC_MARGIN_NUMBER);
	send_sci(SCI_SETMARGINWIDTHN, 0, 42);
	send_sci(SCI_STYLESETBACK, STYLE_LINENUMBER, 0x181818);
	send_sci(SCI_STYLESETFORE, STYLE_LINENUMBER, 0x858585);

	// Soft Tabs (4 espacios, navegación fluida)
	send_sci(SCI_SETTABWIDTH, 4, 0);
	send_sci(SCI_SETUSETABS, 0, 0);
	send_sci(SCI_SETTABINDENTS, 1, 0);
	send_sci(SCI_SETBACKSPACEUNINDENTS, 1, 0);
}

void NovaScriptEditor::setup_autocompletion()
{
	send_sci(SCI_AUTOCSETSEPARATOR, ' ', 0);
	send_sci(SCI_AUTOCSETIGNORECASE, 1, 0);
}

void NovaScriptEditor::show_autocompletion(const std::string& word_list)
{
	int current_pos = send_sci(SCI_GETCURRENTPOS, 0, 0);
	int start_pos = send_sci(SCI_WORDSTARTPOSITION, current_pos, 1);
	int len_entered = current_pos - start_pos;

	send_sci(SCI_AUTOCSHOW, len_entered, (intptr_t)word_list.c_str());
}

void NovaScriptEditor::get_cursor_position(int& line, int& col) const
{
	int pos = send_sci(SCI_GETCURRENTPOS, 0, 0);
	line = send_sci(SCI_LINEFROMPOSITION, pos, 0) + 1;
	int line_start = send_sci(SCI_POSITIONFROMLINE, line - 1, 0);
	col = pos - line_start + 1;
}

void NovaScriptEditor::on_notify(GtkWidget* w, gint param, SCNotification* n, gpointer data)
{
	NovaScriptEditor* self = static_cast<NovaScriptEditor*>(data);
	if (!self || !n) return;

	if (n->nmhdr.code == SCN_UPDATEUI) {
		self->_signal_cursor_changed.emit();
	} else if (n->nmhdr.code == SCN_MODIFIED) {
		if (n->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT)) {
			self->_signal_text_changed.emit();
		}
	} else if (n->nmhdr.code == SCN_MARGINCLICK) {
		if (n->margin == 2) {
			int line_click = self->send_sci(SCI_LINEFROMPOSITION, n->position, 0);
			self->send_sci(SCI_TOGGLEFOLD, line_click, 0);
		}
	}
}