#include <ytk/ytk.h>
#include <Scintilla.h>
#include <SciLexer.h>
#include <ScintillaWidget.h>

#include "nova_script_editor.h"
#include "nova_search_bar.h"
#include "nova_lua_linter.h"
#include "nova_autocomplete.h"

#include <iostream>
#include <algorithm>
#include <cctype>

extern "C" {
	GtkWidget* scintilla_new (void);
	intptr_t   scintilla_send_message (ScintillaObject* sci, unsigned int iMessage, uintptr_t wParam, intptr_t lParam);
}

#ifndef SCINTILLA
#define SCINTILLA(obj) (reinterpret_cast<ScintillaObject*>(obj))
#endif

NovaScriptEditor::NovaScriptEditor()
	: _sci(nullptr)
	, _setting_text(false)
{
	GtkWidget* w = scintilla_new();
	_sci = SCINTILLA(w);

	// Instanciación de componentes modulares
	_search_bar    = new NovaSearchBar(*this);
	_linter        = new NovaLuaLinter(*this);
	_autocomplete  = new NovaAutocomplete(*this);

	// Empaquetar la barra de búsqueda flotante arriba (con visibilidad inteligente)
	_search_bar->set_no_show_all(true);
	this->pack_start(*_search_bar, false, false, 2);
	_search_bar->hide();

	// Empaquetar el widget Scintilla nativo
	gtk_box_pack_start(GTK_BOX(gobj()), w, TRUE, TRUE, 0);
	gtk_widget_show(w);

	// Configurar apariencia y estilos
	set_dark_cyberpunk_theme();
	set_lua_syntax();

	// Configurar autocompletado básico
	send_message(SCI_AUTOCSETSEPARATOR, ' ');
	send_message(SCI_AUTOCSETMAXHEIGHT, 10);
	send_message(SCI_AUTOCSETIGNORECASE, 1);
	send_message(SCI_AUTOCSETCANCELATSTART, 0);

	// Conexión de eventos e interrupciones
	g_signal_connect(_sci, "sci-notify", G_CALLBACK(&NovaScriptEditor::on_notify), this);
	g_signal_connect(_sci, "key-press-event", G_CALLBACK(&NovaScriptEditor::on_key_press), this);
	g_signal_connect(_sci, "scroll-event", G_CALLBACK(&NovaScriptEditor::on_scroll), this);
}

NovaScriptEditor::~NovaScriptEditor()
{
	delete _search_bar;
	delete _linter;
	delete _autocomplete;
}

intptr_t
NovaScriptEditor::send_message(unsigned int msg, uintptr_t wp, intptr_t lp) const
{
	return scintilla_send_message(_sci, msg, wp, lp);
}

void
NovaScriptEditor::set_lua_state(lua_State* L)
{
	_linter->set_lua_state(L);
	_autocomplete->set_lua_state(L);
}

void
NovaScriptEditor::invalidate_globals_cache()
{
	_autocomplete->invalidate_cache();
}

void
NovaScriptEditor::set_text(const std::string& text)
{
	_setting_text = true;
	send_message(SCI_SETTEXT, 0, (intptr_t)text.c_str());
	send_message(SCI_EMPTYUNDOBUFFER);
	_linter->clear_indicators();
	_setting_text = false;
}

std::string
NovaScriptEditor::get_text() const
{
	int len = (int)send_message(SCI_GETLENGTH);
	if (len <= 0) return "";
	std::string buf(len + 1, '\0');
	send_message(SCI_GETTEXT, len + 1, (intptr_t)&buf[0]);
	buf.resize(len);
	return buf;
}

void
NovaScriptEditor::get_cursor_position(int& line, int& col) const
{
	int pos = (int)send_message(SCI_GETCURRENTPOS);
	line = (int)send_message(SCI_LINEFROMPOSITION, pos) + 1;
	col  = (int)send_message(SCI_GETCOLUMN, pos) + 1;
}

void NovaScriptEditor::show_search_bar(bool focus_replace) { _search_bar->show_bar(focus_replace); }
void NovaScriptEditor::hide_search_bar() { _search_bar->hide_bar(); }
void NovaScriptEditor::grab_editor_focus() { gtk_widget_grab_focus(GTK_WIDGET(_sci)); }

void
NovaScriptEditor::set_dark_cyberpunk_theme()
{
	send_message(SCI_STYLECLEARALL, 0, 0);
	send_message(SCI_SETCODEPAGE, SC_CP_UTF8);

	send_message(SCI_STYLESETFONT, STYLE_DEFAULT, (intptr_t)"Monospace");
	send_message(SCI_STYLESETSIZE, STYLE_DEFAULT, 10);
	send_message(SCI_STYLESETFORE, STYLE_DEFAULT, 0xD4D4D4);
	send_message(SCI_STYLESETBACK, STYLE_DEFAULT, 0x1E1E1E);

	send_message(SCI_SETCARETFORE, 0xFFFFFF);
	send_message(SCI_SETCARETWIDTH, 1);
	send_message(SCI_SETCARETLINEVISIBLE, 1);
	send_message(SCI_SETCARETLINEBACK, 0x2A2A2A);
	send_message(SCI_SETCARETPERIOD, 500);

	send_message(SCI_SETMARGINTYPEN, 0, SC_MARGIN_NUMBER);
	send_message(SCI_SETMARGINWIDTHN, 0, 42);
	send_message(SCI_STYLESETBACK, STYLE_LINENUMBER, 0x181818);
	send_message(SCI_STYLESETFORE, STYLE_LINENUMBER, 0x858585);

	send_message(SCI_SETMARGINTYPEN, 2, SC_MARGIN_SYMBOL);
	send_message(SCI_SETMARGINMASKN, 2, SC_MASK_FOLDERS);
	send_message(SCI_SETMARGINWIDTHN, 2, 16);
	send_message(SCI_SETMARGINSENSITIVEN, 2, 1);
	send_message(SCI_SETFOLDFLAGS, SC_FOLDFLAG_LINEAFTER_CONTRACTED, 0);
	send_message(SCI_SETAUTOMATICFOLD, SC_AUTOMATICFOLD_SHOW | SC_AUTOMATICFOLD_CLICK);
	send_message(SCI_SETFOLDEXPANDED, 0, 1);

	send_message(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPEN, SC_MARK_BOXMINUS);
	send_message(SCI_MARKERDEFINE, SC_MARKNUM_FOLDER, SC_MARK_BOXPLUS);
	send_message(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE);
	send_message(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNER);
	send_message(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEREND, SC_MARK_BOXPLUSCONNECTED);
	send_message(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPENMID, SC_MARK_BOXMINUSCONNECTED);
	send_message(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNER);

	for (int m = SC_MARKNUM_FOLDEREND; m <= SC_MARKNUM_FOLDEROPEN; ++m) {
		send_message(SCI_MARKERSETFORE, m, 0x1E1E1E);
		send_message(SCI_MARKERSETBACK, m, 0x858585);
	}

	send_message(SCI_SETTABWIDTH, 4, 0);
	send_message(SCI_SETUSETABS, 0, 0);
	send_message(SCI_SETTABINDENTS, 1, 0);
	send_message(SCI_SETBACKSPACEUNINDENTS, 1, 0);

	send_message(SCI_INDICSETSTYLE, 0, INDIC_SQUIGGLE);
	send_message(SCI_INDICSETFORE, 0, 0x5555FF);
	send_message(SCI_INDICSETUNDER, 0, 1);

	send_message(SCI_CALLTIPSETFORE, 0xD4D4D4);
	send_message(SCI_CALLTIPSETBACK, 0x2A2D2E);
	send_message(SCI_CALLTIPSETFOREHLT, 0x00F0FF);
	send_message(SCI_CALLTIPUSESTYLE, 10);
}

void
NovaScriptEditor::set_lua_syntax()
{
	send_message(SCI_SETLEXER, SCLEX_LUA);

	send_message(SCI_STYLESETFORE, SCE_LUA_WORD, 0x00F0FF);
	send_message(SCI_STYLESETBOLD, SCE_LUA_WORD, 1);
	send_message(SCI_STYLESETFORE, SCE_LUA_COMMENT, 0x6A9955);
	send_message(SCI_STYLESETFORE, SCE_LUA_COMMENTLINE, 0x6A9955);
	send_message(SCI_STYLESETFORE, SCE_LUA_STRING, 0xCE9178);
	send_message(SCI_STYLESETFORE, SCE_LUA_NUMBER, 0xB5CEA8);
	send_message(SCI_STYLESETFORE, SCE_LUA_OPERATOR, 0xD4D4D4);

	send_message(SCI_STYLESETFORE, SCE_LUA_DEFAULT, 0xD4D4D4);
	send_message(SCI_STYLESETFORE, SCE_LUA_IDENTIFIER, 0xD4D4D4);
	send_message(SCI_STYLESETFORE, SCE_LUA_WORD2, 0xDCDCAA);
	send_message(SCI_STYLESETFORE, SCE_LUA_WORD3, 0x4EC9B0);

	for (int i = 0; i <= SCE_LUA_WORD8; ++i) {
		send_message(SCI_STYLESETBACK, i, 0x1E1E1E);
	}

	send_message(SCI_SETKEYWORDS, 0, (intptr_t)
		"and break do else elseif end false for function goto "
		"if in local nil not or repeat return then true until while");
	send_message(SCI_SETKEYWORDS, 1, (intptr_t)
		"print type tostring tonumber pairs ipairs next select "
		"unpack rawget rawset rawequal setmetatable getmetatable "
		"require dofile loadfile load pcall xpcall error assert");
	send_message(SCI_SETKEYWORDS, 2, (intptr_t)
		"math string table os io coroutine debug package ARDOUR Session Editor");
}

void
NovaScriptEditor::do_smart_indent(int ch)
{
	if (ch == '\n' || ch == '\r') {
		int cur_pos = (int)send_message(SCI_GETCURRENTPOS);
		int cur_line = (int)send_message(SCI_LINEFROMPOSITION, cur_pos);
		if (cur_line > 0) {
			int prev_line = cur_line - 1;
			int indent = (int)send_message(SCI_GETLINEINDENTATION, prev_line);
			int prev_len = (int)send_message(SCI_LINELENGTH, prev_line);
			if (prev_len > 0) {
				std::string prev_text(prev_len + 1, '\0');
				send_message(SCI_GETLINE, prev_line, (intptr_t)&prev_text[0]);
				prev_text.resize(prev_len);

				while (!prev_text.empty() && (isspace(prev_text.back()) || prev_text.back() == '\r' || prev_text.back() == '\n')) {
					prev_text.pop_back();
				}

				if (!prev_text.empty()) {
					char last_c = prev_text.back();
					if (last_c == '{' || last_c == '(') {
						indent += 4;
					} else {
						auto ends_with = [&](const std::string& kw) {
							if (prev_text.size() >= kw.size()) {
								std::string end_str = prev_text.substr(prev_text.size() - kw.size());
								if (end_str == kw) {
									if (prev_text.size() == kw.size()) return true;
									char before = prev_text[prev_text.size() - kw.size() - 1];
									return !isalnum(before) && before != '_';
								}
							}
							return false;
						};
						if (ends_with("do") || ends_with("then") || ends_with("repeat") || ends_with("function")) {
							indent += 4;
						}
					}
				}
			}
			send_message(SCI_SETLINEINDENTATION, cur_line, indent);
			send_message(SCI_GOTOPOS, send_message(SCI_GETLINEINDENTPOSITION, cur_line));
		}
	} else if (ch == 'd' || ch == 'D') {
		int cur_pos = (int)send_message(SCI_GETCURRENTPOS);
		int cur_line = (int)send_message(SCI_LINEFROMPOSITION, cur_pos);
		int len = (int)send_message(SCI_LINELENGTH, cur_line);
		if (len > 0) {
			std::string line_text(len + 1, '\0');
			send_message(SCI_GETLINE, cur_line, (intptr_t)&line_text[0]);
			line_text.resize(len);

			std::string trimmed = line_text;
			trimmed.erase(0, trimmed.find_first_not_of(" \t"));
			trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);

			if (trimmed == "end" || trimmed == "end)") {
				int indent = (int)send_message(SCI_GETLINEINDENTATION, cur_line);
				if (indent >= 4) {
					send_message(SCI_SETLINEINDENTATION, cur_line, indent - 4);
				}
			}
		}
	}
}

gboolean
NovaScriptEditor::on_scroll(GtkWidget* /*widget*/, GdkEventScroll* event, gpointer data)
{
	NovaScriptEditor* editor = static_cast<NovaScriptEditor*>(data);
	if (!editor || !event) return FALSE;

	if (event->state & GDK_CONTROL_MASK) {
		if (event->direction == GDK_SCROLL_UP) {
			editor->send_message(SCI_ZOOMIN);
			return TRUE;
		} else if (event->direction == GDK_SCROLL_DOWN) {
			editor->send_message(SCI_ZOOMOUT);
			return TRUE;
		}
	}
	return FALSE;
}

gboolean
NovaScriptEditor::on_key_press(GtkWidget* /*widget*/, GdkEventKey* event, gpointer data)
{
	NovaScriptEditor* editor = static_cast<NovaScriptEditor*>(data);
	if (!editor || !event) return FALSE;

	if (event->keyval == 0xFF1B) { // Escape
		if (editor->_search_bar->is_visible()) {
			editor->hide_search_bar();
			return TRUE;
		}
		return FALSE;
	}

	if (event->state & GDK_CONTROL_MASK) {
		if (event->keyval == 'f' || event->keyval == 'F') {
			editor->show_search_bar(false);
			return TRUE;
		}
		if (event->keyval == 'h' || event->keyval == 'H') {
			editor->show_search_bar(true);
			return TRUE;
		}
		if (event->keyval == '+' || event->keyval == '=' || event->keyval == 0xffab) {
			editor->send_message(SCI_ZOOMIN);
			return TRUE;
		}
		if (event->keyval == '-' || event->keyval == 0xffad) {
			editor->send_message(SCI_ZOOMOUT);
			return TRUE;
		}
		if (event->keyval == '0' || event->keyval == 0xffb0) {
			editor->send_message(SCI_SETZOOM, 0);
			return TRUE;
		}
	}
	return FALSE;
}

void
NovaScriptEditor::on_notify(GtkWidget* /*w*/, gint /*param*/, SCNotification* n, gpointer data)
{
	NovaScriptEditor* editor = static_cast<NovaScriptEditor*>(data);
	if (!editor || !n) return;

	switch (n->nmhdr.code) {
	case SCN_CHARADDED:
		editor->do_smart_indent(n->ch);
		editor->_autocomplete->handle_char(n->ch);
		break;

	case SCN_UPDATEUI:
		editor->_signal_cursor_changed.emit();
		editor->_autocomplete->update_calltip();
		break;

	case SCN_MODIFIED:
		if (!editor->_setting_text && (n->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT))) {
			editor->_signal_text_changed.emit();
			editor->_linter->schedule_lint();
		}
		break;

	default:
		break;
	}
}