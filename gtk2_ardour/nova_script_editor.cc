#include <ytk/ytk.h>
#include <Scintilla.h>
#include <SciLexer.h>
#include <ScintillaWidget.h>

#include "nova_script_editor.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cmath>

#include "lua/luastate.h"
#include "luabridge/LuaBridge.h"

extern "C" {
	GtkWidget* scintilla_new (void);
	intptr_t   scintilla_send_message (ScintillaObject* sci, unsigned int iMessage, uintptr_t wParam, intptr_t lParam);
}

#ifndef SCINTILLA
#define SCINTILLA(obj) (reinterpret_cast<ScintillaObject*>(obj))
#endif

NovaScriptEditor::NovaScriptEditor()
	: _sci(nullptr)
	, _lua_state(nullptr)
	, _replace_visible(false)
	, _match_current(0)
	, _match_total(0)
	, _setting_text(false)
	, _pending_autoc_ch(0)
	, _globals_cache_dirty(true)
	, _calltip_active(false)
	, _calltip_anchor(0)
{
	GtkWidget* w = scintilla_new();
	_sci = SCINTILLA(w);

	/* --- UI de Búsqueda Flotante Arriba --- */
	_btn_toggle_replace.set_label(" > ");
	_btn_find_prev.set_label(" ↑ ");
	_btn_find_next.set_label(" ↓ ");
	_btn_close_search.set_label(" ✕ ");
	_btn_replace.set_label(" Replace ");
	_btn_replace_all.set_label(" All ");

	_search_entry.set_width_chars(20);
	_replace_entry.set_width_chars(20);

	_lbl_match_count.set_text("0 of 0");
	_lbl_match_count.set_size_request(75, -1);

	_find_row.set_spacing(4);
	_find_row.set_border_width(4);
	_find_row.pack_start(_btn_toggle_replace, false, false, 0);
	_find_row.pack_start(_search_entry, false, false, 2);
	_find_row.pack_start(_lbl_match_count, false, false, 4);
	_find_row.pack_start(_btn_find_prev, false, false, 0);
	_find_row.pack_start(_btn_find_next, false, false, 0);
	_find_row.pack_end(_btn_close_search, false, false, 0);

	_replace_row.set_spacing(4);
	_replace_row.set_border_width(4);
	Gtk::Label* spacer = Gtk::manage(new Gtk::Label("    "));
	_replace_row.pack_start(*spacer, false, false, 0);
	_replace_row.pack_start(_replace_entry, false, false, 2);
	_replace_row.pack_start(_btn_replace, false, false, 2);
	_replace_row.pack_start(_btn_replace_all, false, false, 0);

	_search_panel.set_spacing(2);
	_search_panel.set_border_width(4);
	_search_panel.pack_start(_find_row, false, false, 0);
	_search_panel.pack_start(_replace_row, false, false, 0);

	_search_frame.set_shadow_type(Gtk::SHADOW_ETCHED_OUT);
	_search_frame.add(_search_panel);

	_top_overlay_bar.pack_end(_search_frame, false, false, 12);

	_btn_toggle_replace.signal_clicked().connect(sigc::mem_fun(*this, &NovaScriptEditor::toggle_replace_row));
	_btn_find_next.signal_clicked().connect(sigc::mem_fun(*this, &NovaScriptEditor::do_find_next));
	_btn_find_prev.signal_clicked().connect(sigc::mem_fun(*this, &NovaScriptEditor::do_find_prev));
	_btn_replace.signal_clicked().connect(sigc::mem_fun(*this, &NovaScriptEditor::do_replace));
	_btn_replace_all.signal_clicked().connect(sigc::mem_fun(*this, &NovaScriptEditor::do_replace_all));
	_btn_close_search.signal_clicked().connect(sigc::mem_fun(*this, &NovaScriptEditor::hide_search_bar));

	_search_entry.signal_activate().connect(sigc::mem_fun(*this, &NovaScriptEditor::do_find_next));
	_replace_entry.signal_activate().connect(sigc::mem_fun(*this, &NovaScriptEditor::do_replace));
	_search_entry.signal_changed().connect(sigc::mem_fun(*this, &NovaScriptEditor::on_search_text_changed));

	_top_overlay_bar.set_no_show_all(true);
	_search_frame.set_no_show_all(true);

	this->pack_start(_top_overlay_bar, false, false, 2);
	gtk_box_pack_start(GTK_BOX(gobj()), w, TRUE, TRUE, 0);

	gtk_widget_show(w);

	_replace_row.hide();
	_search_frame.hide();
	_top_overlay_bar.hide();

	set_dark_cyberpunk_theme();
	set_lua_syntax();
	setup_autocompletion();
	init_native_api_dictionary();
	init_calltip_dictionary();

	g_signal_connect(_sci, "sci-notify", G_CALLBACK(&NovaScriptEditor::on_notify), this);
	g_signal_connect(_sci, "key-press-event", G_CALLBACK(&NovaScriptEditor::on_scintilla_key_press), this);
	g_signal_connect(_sci, "scroll-event", G_CALLBACK(&NovaScriptEditor::on_scintilla_scroll), this);
	g_signal_connect(_search_entry.gobj(), "key-press-event", G_CALLBACK(&NovaScriptEditor::on_search_entry_key), this);
}

NovaScriptEditor::~NovaScriptEditor()
{
	if (_lint_timeout_connection.connected()) {
		_lint_timeout_connection.disconnect();
	}
	if (_autoc_timeout_connection.connected()) {
		_autoc_timeout_connection.disconnect();
	}
}

intptr_t
NovaScriptEditor::send_sci(unsigned int msg, uintptr_t wp, intptr_t lp) const
{
	return scintilla_send_message(_sci, msg, wp, lp);
}

gboolean
NovaScriptEditor::on_scintilla_scroll(GtkWidget* /*widget*/, GdkEventScroll* event, gpointer data)
{
	NovaScriptEditor* editor = static_cast<NovaScriptEditor*>(data);
	if (!editor || !event) return FALSE;

	if (event->state & GDK_CONTROL_MASK) {
		if (event->direction == GDK_SCROLL_UP) {
			editor->send_sci(SCI_ZOOMIN);
			return TRUE;
		} else if (event->direction == GDK_SCROLL_DOWN) {
			editor->send_sci(SCI_ZOOMOUT);
			return TRUE;
		}
	}
	return FALSE;
}

gboolean
NovaScriptEditor::on_scintilla_key_press(GtkWidget* /*widget*/, GdkEventKey* event, gpointer data)
{
	NovaScriptEditor* editor = static_cast<NovaScriptEditor*>(data);
	if (!editor || !event) {
		return FALSE;
	}

	if (event->keyval == 0xFF1B) {
		if (editor->_search_frame.is_visible()) {
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
			editor->send_sci(SCI_ZOOMIN);
			return TRUE;
		}
		if (event->keyval == '-' || event->keyval == 0xffad) {
			editor->send_sci(SCI_ZOOMOUT);
			return TRUE;
		}
		if (event->keyval == '0' || event->keyval == 0xffb0) {
			editor->send_sci(SCI_SETZOOM, 0);
			return TRUE;
		}
	}

	return FALSE;
}

gboolean
NovaScriptEditor::on_search_entry_key(GtkWidget* /*widget*/, GdkEventKey* event, gpointer data)
{
	NovaScriptEditor* editor = static_cast<NovaScriptEditor*>(data);
	if (!editor || !event) {
		return FALSE;
	}

	if (event->keyval == 0xFF1B) {
		editor->hide_search_bar();
		return TRUE;
	}

	if (event->keyval == 0xFFC0) {
		if (event->state & GDK_SHIFT_MASK) {
			editor->do_find_prev();
		} else {
			editor->do_find_next();
		}
		return TRUE;
	}

	return FALSE;
}

void
NovaScriptEditor::toggle_replace_row()
{
	_replace_visible = !_replace_visible;
	if (_replace_visible) {
		_btn_toggle_replace.set_label(" v ");
		_replace_row.show_all();
	} else {
		_btn_toggle_replace.set_label(" > ");
		_replace_row.hide();
	}
}

void
NovaScriptEditor::show_search_bar(bool focus_replace)
{
	_top_overlay_bar.show();
	_search_frame.show_all();

	if (focus_replace) {
		_replace_visible = true;
		_btn_toggle_replace.set_label(" v ");
		_replace_row.show_all();
	} else if (!_replace_visible) {
		_replace_row.hide();
	}

	int sel_len = (int)send_sci(SCI_GETSELTEXT, 0, 0);
	if (sel_len > 1) {
		std::string sel(sel_len, '\0');
		send_sci(SCI_GETSELTEXT, 0, (intptr_t)&sel[0]);
		sel.resize(sel_len - 1);
		if (!sel.empty() && sel.find('\n') == std::string::npos) {
			_search_entry.set_text(sel);
		}
	}

	if (focus_replace) {
		_replace_entry.grab_focus();
		_replace_entry.select_region(0, -1);
	} else {
		_search_entry.grab_focus();
		_search_entry.select_region(0, -1);
	}

	update_match_count();
}

void
NovaScriptEditor::hide_search_bar()
{
	_search_frame.hide();
	_top_overlay_bar.hide();
	_match_current = 0;
	_match_total   = 0;
	_lbl_match_count.set_text("0 of 0");
	gtk_widget_grab_focus(GTK_WIDGET(_sci));
}

void
NovaScriptEditor::on_search_text_changed()
{
	update_match_count();
}

int
NovaScriptEditor::count_all_matches(const std::string& term)
{
	if (term.empty()) {
		return 0;
	}

	const int doc_len = (int)send_sci(SCI_GETLENGTH);
	if (doc_len <= 0) {
		return 0;
	}

	const int old_start = (int)send_sci(SCI_GETSELECTIONSTART);
	const int old_end   = (int)send_sci(SCI_GETSELECTIONEND);

	int count = 0;
	int pos = 0;
	while (pos < doc_len) {
		send_sci(SCI_SETTARGETSTART, pos);
		send_sci(SCI_SETTARGETEND, doc_len);
		send_sci(SCI_SETSEARCHFLAGS, 0);
		int found = (int)send_sci(SCI_SEARCHINTARGET, term.size(), (intptr_t)term.c_str());
		if (found == -1) {
			break;
		}
		++count;
		int t_end = (int)send_sci(SCI_GETTARGETEND);
		if (t_end <= pos) {
			break;
		}
		pos = t_end;
	}

	send_sci(SCI_SETSELECTIONSTART, old_start);
	send_sci(SCI_SETSELECTIONEND, old_end);
	return count;
}

void
NovaScriptEditor::update_match_count()
{
	std::string term = _search_entry.get_text();
	if (term.empty()) {
		_match_total   = 0;
		_match_current = 0;
		_lbl_match_count.set_text("0 of 0");
		return;
	}

	_match_total = count_all_matches(term);

	if (_match_total == 0) {
		_match_current = 0;
		_lbl_match_count.set_text("0 of 0");
		return;
	}

	const int cur = (int)send_sci(SCI_GETCURRENTPOS);
	const int doc_len = (int)send_sci(SCI_GETLENGTH);
	int idx = 0;
	int pos = 0;
	int current_idx = 0;

	while (pos < doc_len) {
		send_sci(SCI_SETTARGETSTART, pos);
		send_sci(SCI_SETTARGETEND, doc_len);
		send_sci(SCI_SETSEARCHFLAGS, 0);
		int found = (int)send_sci(SCI_SEARCHINTARGET, term.size(), (intptr_t)term.c_str());
		if (found == -1) {
			break;
		}
		++idx;
		int t_start = (int)send_sci(SCI_GETTARGETSTART);
		int t_end   = (int)send_sci(SCI_GETTARGETEND);
		if (t_start <= cur && cur <= t_end) {
			current_idx = idx;
		} else if (t_start > cur && current_idx == 0) {
			current_idx = idx;
			break;
		}
		if (t_end <= pos) {
			break;
		}
		pos = t_end;
	}

	if (current_idx == 0) {
		current_idx = 1;
	}
	_match_current = current_idx;

	char buf[64];
	snprintf(buf, sizeof(buf), "%d of %d", _match_current, _match_total);
	_lbl_match_count.set_text(buf);
}

bool
NovaScriptEditor::find_with_dir(bool forward)
{
	std::string term = _search_entry.get_text();
	if (term.empty()) {
		return false;
	}

	const int doc_len = (int)send_sci(SCI_GETLENGTH);
	if (doc_len <= 0) {
		return false;
	}

	int from;
	if (forward) {
		from = (int)send_sci(SCI_GETSELECTIONEND);
	} else {
		from = (int)send_sci(SCI_GETSELECTIONSTART);
	}

	int found = -1;
	int t_start = -1;
	int t_end   = -1;

	if (forward) {
		send_sci(SCI_SETTARGETSTART, from);
		send_sci(SCI_SETTARGETEND, doc_len);
		send_sci(SCI_SETSEARCHFLAGS, 0);
		found = (int)send_sci(SCI_SEARCHINTARGET, term.size(), (intptr_t)term.c_str());
		if (found == -1) {
			send_sci(SCI_SETTARGETSTART, 0);
			send_sci(SCI_SETTARGETEND, doc_len);
			found = (int)send_sci(SCI_SEARCHINTARGET, term.size(), (intptr_t)term.c_str());
		}
		if (found != -1) {
			t_start = (int)send_sci(SCI_GETTARGETSTART);
			t_end   = (int)send_sci(SCI_GETTARGETEND);
		}
	} else {
		int pos = 0;
		int last_start = -1;
		int last_end   = -1;
		while (pos < from) {
			send_sci(SCI_SETTARGETSTART, pos);
			send_sci(SCI_SETTARGETEND, from);
			send_sci(SCI_SETSEARCHFLAGS, 0);
			int f = (int)send_sci(SCI_SEARCHINTARGET, term.size(), (intptr_t)term.c_str());
			if (f == -1) {
				break;
			}
			last_start = (int)send_sci(SCI_GETTARGETSTART);
			last_end   = (int)send_sci(SCI_GETTARGETEND);
			if (last_end <= pos) {
				break;
			}
			pos = last_end;
		}
		if (last_start >= 0) {
			t_start = last_start;
			t_end   = last_end;
			found   = last_start;
		} else {
			pos = 0;
			last_start = -1;
			last_end   = -1;
			while (pos < doc_len) {
				send_sci(SCI_SETTARGETSTART, pos);
				send_sci(SCI_SETTARGETEND, doc_len);
				send_sci(SCI_SETSEARCHFLAGS, 0);
				int f = (int)send_sci(SCI_SEARCHINTARGET, term.size(), (intptr_t)term.c_str());
				if (f == -1) {
					break;
				}
				last_start = (int)send_sci(SCI_GETTARGETSTART);
				last_end   = (int)send_sci(SCI_GETTARGETEND);
				if (last_end <= pos) {
					break;
				}
				pos = last_end;
			}
			if (last_start >= 0) {
				t_start = last_start;
				t_end   = last_end;
				found   = last_start;
			}
		}
	}

	if (found == -1) {
		_lbl_match_count.set_text("0 of 0");
		_match_current = 0;
		_match_total   = 0;
		return false;
	}

	send_sci(SCI_SETSELECTIONSTART, t_start);
	send_sci(SCI_SETSELECTIONEND, t_end);
	send_sci(SCI_SCROLLCARET);

	_match_total = count_all_matches(term);
	int idx = 0;
	int pos = 0;
	_match_current = 1;
	while (pos < doc_len) {
		send_sci(SCI_SETTARGETSTART, pos);
		send_sci(SCI_SETTARGETEND, doc_len);
		send_sci(SCI_SETSEARCHFLAGS, 0);
		int f = (int)send_sci(SCI_SEARCHINTARGET, term.size(), (intptr_t)term.c_str());
		if (f == -1) {
			break;
		}
		++idx;
		if ((int)send_sci(SCI_GETTARGETSTART) == t_start) {
			_match_current = idx;
			break;
		}
		int te = (int)send_sci(SCI_GETTARGETEND);
		if (te <= pos) {
			break;
		}
		pos = te;
	}

	char buf[64];
	snprintf(buf, sizeof(buf), "%d of %d", _match_current, _match_total);
	_lbl_match_count.set_text(buf);

	send_sci(SCI_SETSELECTIONSTART, t_start);
	send_sci(SCI_SETSELECTIONEND, t_end);
	return true;
}

void
NovaScriptEditor::do_find_next()
{
	find_with_dir(true);
}

void
NovaScriptEditor::do_find_prev()
{
	find_with_dir(false);
}

void
NovaScriptEditor::do_replace()
{
	std::string term = _search_entry.get_text();
	std::string repl = _replace_entry.get_text();
	if (term.empty()) {
		return;
	}

	int sel_len = (int)send_sci(SCI_GETSELTEXT, 0, 0);
	if (sel_len > 1) {
		std::string sel(sel_len, '\0');
		send_sci(SCI_GETSELTEXT, 0, (intptr_t)&sel[0]);
		sel.resize(sel_len - 1);

		std::string a = sel;
		std::string b = term;
		std::transform(a.begin(), a.end(), a.begin(), ::tolower);
		std::transform(b.begin(), b.end(), b.begin(), ::tolower);
		if (a == b) {
			send_sci(SCI_REPLACESEL, 0, (intptr_t)repl.c_str());
		}
	}

	do_find_next();
}

void
NovaScriptEditor::do_replace_all()
{
	std::string term = _search_entry.get_text();
	std::string repl = _replace_entry.get_text();
	if (term.empty()) {
		return;
	}

	const int doc_len = (int)send_sci(SCI_GETLENGTH);
	if (doc_len <= 0) {
		return;
	}

	send_sci(SCI_BEGINUNDOACTION);

	int replacements = 0;
	int pos = 0;
	for (;;) {
		int len_now = (int)send_sci(SCI_GETLENGTH);
		if (pos >= len_now) {
			break;
		}

		send_sci(SCI_SETTARGETSTART, pos);
		send_sci(SCI_SETTARGETEND, len_now);
		send_sci(SCI_SETSEARCHFLAGS, 0);
		int found = (int)send_sci(SCI_SEARCHINTARGET, term.size(), (intptr_t)term.c_str());
		if (found == -1) {
			break;
		}

		int t_start = (int)send_sci(SCI_GETTARGETSTART);
		send_sci(SCI_REPLACETARGET, repl.size(), (intptr_t)repl.c_str());
		++replacements;
		pos = t_start + (int)repl.size();
	}

	send_sci(SCI_ENDUNDOACTION);

	char buf[64];
	snprintf(buf, sizeof(buf), "R: %d", replacements);
	_lbl_match_count.set_text(buf);
	_match_current = 0;
	_match_total   = 0;
}

void
NovaScriptEditor::set_dark_cyberpunk_theme()
{
	send_sci(SCI_STYLECLEARALL, 0, 0);
	send_sci(SCI_SETCODEPAGE, SC_CP_UTF8);

	send_sci(SCI_STYLESETFONT, STYLE_DEFAULT, (intptr_t)"Monospace");
	send_sci(SCI_STYLESETSIZE, STYLE_DEFAULT, 10);
	send_sci(SCI_STYLESETFORE, STYLE_DEFAULT, 0xD4D4D4);
	send_sci(SCI_STYLESETBACK, STYLE_DEFAULT, 0x1E1E1E);

	send_sci(SCI_SETCARETFORE, 0xFFFFFF);
	send_sci(SCI_SETCARETWIDTH, 1);
	send_sci(SCI_SETCARETLINEVISIBLE, 1);
	send_sci(SCI_SETCARETLINEBACK, 0x2A2A2A);
	send_sci(SCI_SETCARETPERIOD, 500);

	send_sci(SCI_SETMARGINTYPEN, 0, SC_MARGIN_NUMBER);
	send_sci(SCI_SETMARGINWIDTHN, 0, 42);
	send_sci(SCI_STYLESETBACK, STYLE_LINENUMBER, 0x181818);
	send_sci(SCI_STYLESETFORE, STYLE_LINENUMBER, 0x858585);

	send_sci(SCI_SETMARGINTYPEN, 2, SC_MARGIN_SYMBOL);
	send_sci(SCI_SETMARGINMASKN, 2, SC_MASK_FOLDERS);
	send_sci(SCI_SETMARGINWIDTHN, 2, 16);
	send_sci(SCI_SETMARGINSENSITIVEN, 2, 1);
	send_sci(SCI_SETFOLDFLAGS, SC_FOLDFLAG_LINEAFTER_CONTRACTED, 0);
	send_sci(SCI_SETAUTOMATICFOLD, SC_AUTOMATICFOLD_SHOW | SC_AUTOMATICFOLD_CLICK);
	send_sci(SCI_SETFOLDEXPANDED, 0, 1);

	send_sci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPEN, SC_MARK_BOXMINUS);
	send_sci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDER, SC_MARK_BOXPLUS);
	send_sci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE);
	send_sci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNER);
	send_sci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEREND, SC_MARK_BOXPLUSCONNECTED);
	send_sci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPENMID, SC_MARK_BOXMINUSCONNECTED);
	send_sci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNER);

	for (int m = SC_MARKNUM_FOLDEREND; m <= SC_MARKNUM_FOLDEROPEN; ++m) {
		send_sci(SCI_MARKERSETFORE, m, 0x1E1E1E);
		send_sci(SCI_MARKERSETBACK, m, 0x858585);
	}

	send_sci(SCI_SETTABWIDTH, 4, 0);
	send_sci(SCI_SETUSETABS, 0, 0);
	send_sci(SCI_SETTABINDENTS, 1, 0);
	send_sci(SCI_SETBACKSPACEUNINDENTS, 1, 0);

	send_sci(SCI_INDICSETSTYLE, 0, INDIC_SQUIGGLE);
	send_sci(SCI_INDICSETFORE, 0, 0x5555FF);
	send_sci(SCI_INDICSETUNDER, 0, 1);

	send_sci(SCI_CALLTIPSETFORE, 0xD4D4D4);
	send_sci(SCI_CALLTIPSETBACK, 0x2A2D2E);
	send_sci(SCI_CALLTIPSETFOREHLT, 0x00F0FF);
	send_sci(SCI_CALLTIPUSESTYLE, 10);
}

void
NovaScriptEditor::set_lua_syntax()
{
	send_sci(SCI_SETLEXER, SCLEX_LUA);

	send_sci(SCI_STYLESETFORE, SCE_LUA_WORD, 0x00F0FF);
	send_sci(SCI_STYLESETBOLD, SCE_LUA_WORD, 1);
	send_sci(SCI_STYLESETFORE, SCE_LUA_COMMENT, 0x6A9955);
	send_sci(SCI_STYLESETFORE, SCE_LUA_COMMENTLINE, 0x6A9955);
	send_sci(SCI_STYLESETFORE, SCE_LUA_STRING, 0xCE9178);
	send_sci(SCI_STYLESETFORE, SCE_LUA_NUMBER, 0xB5CEA8);
	send_sci(SCI_STYLESETFORE, SCE_LUA_OPERATOR, 0xD4D4D4);

	send_sci(SCI_STYLESETFORE, SCE_LUA_DEFAULT, 0xD4D4D4);
	send_sci(SCI_STYLESETFORE, SCE_LUA_IDENTIFIER, 0xD4D4D4);
	send_sci(SCI_STYLESETFORE, SCE_LUA_WORD2, 0xDCDCAA);
	send_sci(SCI_STYLESETFORE, SCE_LUA_WORD3, 0x4EC9B0);

	for (int i = 0; i <= SCE_LUA_WORD8; ++i) {
		send_sci(SCI_STYLESETBACK, i, 0x1E1E1E);
	}

	send_sci(SCI_SETKEYWORDS, 0, (intptr_t)
		"and break do else elseif end false for function goto "
		"if in local nil not or repeat return then true until while");
	send_sci(SCI_SETKEYWORDS, 1, (intptr_t)
		"print type tostring tonumber pairs ipairs next select "
		"unpack rawget rawset rawequal setmetatable getmetatable "
		"require dofile loadfile load pcall xpcall error assert");
	send_sci(SCI_SETKEYWORDS, 2, (intptr_t)
		"math string table os io coroutine debug package ARDOUR Session Editor");
}

void
NovaScriptEditor::set_lua_state(lua_State* L)
{
	_lua_state = L;
	invalidate_globals_cache();
}

void
NovaScriptEditor::invalidate_globals_cache()
{
	_globals_cache_dirty = true;
	_cached_globals_list.clear();
}

void
NovaScriptEditor::set_text(const std::string& text)
{
	_setting_text = true;
	send_sci(SCI_SETTEXT, 0, (intptr_t)text.c_str());
	send_sci(SCI_EMPTYUNDOBUFFER);
	clear_lint_indicators();
	_setting_text = false;
}

std::string
NovaScriptEditor::get_text() const
{
	int len = (int)send_sci(SCI_GETLENGTH);
	if (len <= 0) {
		return "";
	}
	std::string buf(len + 1, '\0');
	send_sci(SCI_GETTEXT, len + 1, (intptr_t)&buf[0]);
	buf.resize(len);
	return buf;
}

void
NovaScriptEditor::get_cursor_position(int& line, int& col) const
{
	int pos = (int)send_sci(SCI_GETCURRENTPOS);
	line = (int)send_sci(SCI_LINEFROMPOSITION, pos) + 1;
	col  = (int)send_sci(SCI_GETCOLUMN, pos) + 1;
}

void
NovaScriptEditor::setup_autocompletion()
{
	send_sci(SCI_AUTOCSETSEPARATOR, ' ');
	send_sci(SCI_AUTOCSETMAXHEIGHT, 10);
	send_sci(SCI_AUTOCSETIGNORECASE, 1);
	send_sci(SCI_AUTOCSETCANCELATSTART, 0);
}

void
NovaScriptEditor::show_autocompletion(int len_entered, const std::string& word_list)
{
	if (word_list.empty()) {
		send_sci(SCI_AUTOCCANCEL);
		return;
	}
	send_sci(SCI_AUTOCSHOW, len_entered, (intptr_t)word_list.c_str());
}

void
NovaScriptEditor::init_native_api_dictionary()
{
	_native_api_dict["Session"] =
		"config get_tracks instance sample_rate tempo_map "
		"transport_locate transport_rolling transport_sample transport_start transport_stop";

	_native_api_dict["Editor"] =
		"get_selection instance";

	_native_api_dict["AudioTrack"] =
		"gain mute n_channels name solo";

	_native_api_dict["AudioBuffer"] =
		"apply_gain clear data read_from silence size write_to";

	_native_api_dict["MidiBuffer"] =
		"clear events size";

	_native_api_dict["TempoMap"] =
		"meter_at tempo_at";

	_native_api_dict["Plugin"] =
		"name parameter set_parameter";

	_native_api_dict["math"] =
		"abs acos asin atan ceil cos exp floor huge log max min pi "
		"random randomseed sin sqrt tan tanh";

	_native_api_dict["string"] =
		"byte char find format gsub len lower match rep reverse sub upper";

	_native_api_dict["table"] =
		"concat insert remove sort unpack";

	_native_api_dict["os"] =
		"clock date time";

	_native_api_dict["io"] =
		"close open read write";

	_global_word_list =
		"AudioBuffer AudioTrack Editor MidiBuffer Plugin Session TempoMap "
		"assert error ipairs math next os pairs pcall print "
		"rawget rawset require select string table tonumber tostring type unpack xpcall";
}

void
NovaScriptEditor::init_calltip_dictionary()
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

void
NovaScriptEditor::show_calltip(int pos_paren)
{
	std::string func_name = get_word_before_pos(pos_paren, true);
	if (func_name.empty()) {
		return;
	}

	auto it = _calltip_dict.find(func_name);
	if (it == _calltip_dict.end()) {
		return;
	}

	_calltip_signature = it->second;
	_calltip_anchor    = pos_paren;
	_calltip_active    = true;

	send_sci(SCI_CALLTIPSHOW, pos_paren, (intptr_t)_calltip_signature.c_str());
	send_sci(SCI_CALLTIPSETFORE, 0xD4D4D4);
	send_sci(SCI_CALLTIPSETBACK, 0x2A2D2E);
	send_sci(SCI_CALLTIPSETFOREHLT, 0x00F0FF);

	update_calltip_highlight();
}

void
NovaScriptEditor::cancel_calltip()
{
	if (!_calltip_active) {
		return;
	}
	send_sci(SCI_CALLTIPCANCEL);
	_calltip_active = false;
	_calltip_anchor = 0;
	_calltip_signature.clear();
}

void
NovaScriptEditor::update_calltip_highlight()
{
	if (!_calltip_active) {
		return;
	}

	int cur_pos = (int)send_sci(SCI_GETCURRENTPOS);
	if (cur_pos <= _calltip_anchor) {
		cancel_calltip();
		return;
	}

	int comma_count = 0;
	int paren_depth = 0;
	for (int i = _calltip_anchor + 1; i < cur_pos; ++i) {
		int ch = (int)send_sci(SCI_GETCHARAT, i);
		if (ch == '(') {
			++paren_depth;
		} else if (ch == ')') {
			if (paren_depth == 0) {
				cancel_calltip();
				return;
			}
			--paren_depth;
		} else if (ch == ',' && paren_depth == 0) {
			++comma_count;
		}
	}

	size_t open_p  = _calltip_signature.find('(');
	size_t close_p = _calltip_signature.rfind(')');
	if (open_p == std::string::npos || close_p == std::string::npos || close_p <= open_p + 1) {
		send_sci(SCI_CALLTIPSETHLT, 0, 0);
		return;
	}

	std::string params = _calltip_signature.substr(open_p + 1, close_p - open_p - 1);
	std::vector<std::pair<int, int> > param_ranges;
	int pstart = 0;
	int depth  = 0;
	for (int i = 0; i < (int)params.size(); ++i) {
		char c = params[i];
		if (c == '(' || c == '[') {
			++depth;
		} else if (c == ')' || c == ']') {
			--depth;
		} else if (c == ',' && depth == 0) {
			param_ranges.push_back(std::make_pair(pstart, i));
			pstart = i + 1;
		}
	}
	param_ranges.push_back(std::make_pair(pstart, (int)params.size()));

	int active = std::min(comma_count, (int)param_ranges.size() - 1);
	if (active < 0) {
		active = 0;
	}

	int hl_start = (int)open_p + 1 + param_ranges[active].first;
	int hl_end   = (int)open_p + 1 + param_ranges[active].second;

	while (hl_start < hl_end && _calltip_signature[hl_start] == ' ') {
		++hl_start;
	}
	while (hl_end > hl_start && _calltip_signature[hl_end - 1] == ' ') {
		--hl_end;
	}

	send_sci(SCI_CALLTIPSETHLT, hl_start, hl_end);
}

std::string
NovaScriptEditor::get_word_before_pos(int pos, bool include_dots)
{
	if (pos <= 0) {
		return "";
	}
	int start = pos;
	while (start > 0) {
		int ch = (int)send_sci(SCI_GETCHARAT, start - 1);
		bool valid = std::isalnum(ch) || ch == '_';
		if (include_dots && (ch == '.' || ch == ':')) {
			valid = true;
		}
		if (!valid) {
			break;
		}
		--start;
	}
	if (start >= pos) {
		return "";
	}
	std::string word;
	for (int i = start; i < pos; ++i) {
		word += (char)send_sci(SCI_GETCHARAT, i);
	}
	return word;
}

std::string
NovaScriptEditor::get_members_for_class(const std::string& class_name)
{
	auto it = _native_api_dict.find(class_name);
	if (it != _native_api_dict.end()) {
		return it->second;
	}

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
				if (!list.empty()) {
					list += " ";
				}
				list += k;
			}
			return list;
		}
	}
	return "";
}

void
NovaScriptEditor::collect_table_keys(lua_State* L, int index, std::set<std::string>& results, int depth)
{
	if (depth > 2) {
		return;
	}
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
NovaScriptEditor::get_all_globals()
{
	if (!_globals_cache_dirty && !_cached_globals_list.empty()) {
		return _cached_globals_list;
	}

	std::set<std::string> all_words;
	std::istringstream iss(_global_word_list);
	std::string item;
	while (iss >> item) {
		all_words.insert(item);
	}

	if (_lua_state) {
		lua_pushglobaltable(_lua_state);
		collect_table_keys(_lua_state, -1, all_words, 0);
		lua_pop(_lua_state, 1);
	}

	std::string merged;
	for (auto const& w : all_words) {
		if (!merged.empty()) {
			merged += " ";
		}
		merged += w;
	}

	_cached_globals_list = merged;
	_globals_cache_dirty = false;
	return _cached_globals_list;
}

void
NovaScriptEditor::schedule_autocompletion(int ch)
{
	_pending_autoc_ch = ch;
	if (_autoc_timeout_connection.connected()) {
		_autoc_timeout_connection.disconnect();
	}
	_autoc_timeout_connection = Glib::signal_timeout().connect(
		sigc::mem_fun(*this, &NovaScriptEditor::trigger_autocompletion_deferred), 80);
}

bool
NovaScriptEditor::trigger_autocompletion_deferred()
{
	_autoc_timeout_connection.disconnect();
	int pos = (int)send_sci(SCI_GETCURRENTPOS);

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
NovaScriptEditor::do_smart_indent(int ch)
{
	if (ch == '\n' || ch == '\r') {
		int cur_pos = (int)send_sci(SCI_GETCURRENTPOS);
		int cur_line = (int)send_sci(SCI_LINEFROMPOSITION, cur_pos);
		if (cur_line > 0) {
			int prev_line = cur_line - 1;
			int indent = (int)send_sci(SCI_GETLINEINDENTATION, prev_line);

			int prev_len = (int)send_sci(SCI_LINELENGTH, prev_line);
			if (prev_len > 0) {
				std::string prev_text(prev_len + 1, '\0');
				send_sci(SCI_GETLINE, prev_line, (intptr_t)&prev_text[0]);
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

			send_sci(SCI_SETLINEINDENTATION, cur_line, indent);
			send_sci(SCI_GOTOPOS, send_sci(SCI_GETLINEINDENTPOSITION, cur_line));
		}
	}
	else if (ch == 'd' || ch == 'D') {
		int cur_pos = (int)send_sci(SCI_GETCURRENTPOS);
		int cur_line = (int)send_sci(SCI_LINEFROMPOSITION, cur_pos);
		int len = (int)send_sci(SCI_LINELENGTH, cur_line);
		
		if (len > 0) {
			std::string line_text(len + 1, '\0');
			send_sci(SCI_GETLINE, cur_line, (intptr_t)&line_text[0]);
			line_text.resize(len);

			std::string trimmed = line_text;
			trimmed.erase(0, trimmed.find_first_not_of(" \t"));
			trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);

			if (trimmed == "end" || trimmed == "end)") {
				int indent = (int)send_sci(SCI_GETLINEINDENTATION, cur_line);
				if (indent >= 4) {
					send_sci(SCI_SETLINEINDENTATION, cur_line, indent - 4);
				}
			}
		}
	}
}

void
NovaScriptEditor::handle_char_added(int ch)
{
	do_smart_indent(ch);

	if (ch == '(') {
		int pos = (int)send_sci(SCI_GETCURRENTPOS);
		send_sci(SCI_AUTOCCANCEL);
		show_calltip(pos - 1);
		return;
	}

	if (ch == ',') {
		if (_calltip_active) {
			update_calltip_highlight();
		}
		return;
	}

	if (ch == ')') {
		cancel_calltip();
		return;
	}

	if (ch == '\n' || ch == '\r') {
		cancel_calltip();
		return;
	}

	if (!_calltip_active) {
		schedule_autocompletion(ch);
	}
}

void
NovaScriptEditor::schedule_lint()
{
	if (_lint_timeout_connection.connected()) {
		_lint_timeout_connection.disconnect();
	}
	_lint_timeout_connection = Glib::signal_timeout().connect(
		sigc::mem_fun(*this, &NovaScriptEditor::run_lint), 300);
}

bool
NovaScriptEditor::run_lint()
{
	_lint_timeout_connection.disconnect();
	if (!_lua_state) {
		return false;
	}

	clear_lint_indicators();

	std::string code = get_text();
	if (code.empty()) {
		_signal_lint_status.emit(true, "✔ Syntax OK", 0);
		return false;
	}

	int err = luaL_loadbuffer(_lua_state, code.c_str(), code.size(), "@nova_editor");
	if (err == 0) {
		lua_pop(_lua_state, 1);
		_signal_lint_status.emit(true, "✔ Syntax OK", 0);
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
			} catch (...) {
			}
			err_msg = msg.substr(colon2 + 2);
		}
	}

	std::string safe_msg;
	for (char c : err_msg) {
		if (c == '<') {
			safe_msg += "&lt;";
		} else if (c == '>') {
			safe_msg += "&gt;";
		} else if (c == '&') {
			safe_msg += "&amp;";
		} else {
			safe_msg += c;
		}
	}

	int line = std::max(0, err_line - 1);
	int line_start = (int)send_sci(SCI_POSITIONFROMLINE, line);
	int line_end   = (int)send_sci(SCI_GETLINEENDPOSITION, line);

	send_sci(SCI_SETINDICATORCURRENT, 0);
	send_sci(SCI_INDICATORFILLRANGE, line_start, line_end - line_start);

	char buf[512];
	snprintf(buf, sizeof(buf), "✖ Line %d: %s", err_line, safe_msg.c_str());
	_signal_lint_status.emit(false, buf, err_line);
	return false;
}

void
NovaScriptEditor::clear_lint_indicators()
{
	int len = (int)send_sci(SCI_GETLENGTH);
	send_sci(SCI_SETINDICATORCURRENT, 0);
	send_sci(SCI_INDICATORCLEARRANGE, 0, len);
}

void
NovaScriptEditor::on_notify(GtkWidget* /*w*/, gint /*param*/, SCNotification* n, gpointer data)
{
	NovaScriptEditor* editor = static_cast<NovaScriptEditor*>(data);
	if (!editor || !n) {
		return;
	}

	switch (n->nmhdr.code) {
	case SCN_CHARADDED:
		editor->handle_char_added(n->ch);
		break;

	case SCN_UPDATEUI:
		editor->_signal_cursor_changed.emit();
		if (editor->_calltip_active) {
			int cur_pos  = (int)editor->send_sci(SCI_GETCURRENTPOS);
			int cur_line = (int)editor->send_sci(SCI_LINEFROMPOSITION, cur_pos);
			int anc_line = (int)editor->send_sci(SCI_LINEFROMPOSITION, editor->_calltip_anchor);

			if (cur_line != anc_line || cur_pos <= editor->_calltip_anchor) {
				editor->cancel_calltip();
			} else {
				editor->update_calltip_highlight();
			}
		}
		break;

	case SCN_MODIFIED:
		if (!editor->_setting_text && (n->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT))) {
			editor->_signal_text_changed.emit();
			editor->schedule_lint();
		}
		break;

	default:
		break;
	}
}