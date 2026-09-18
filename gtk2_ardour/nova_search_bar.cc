#include <ytk/ytk.h>
#include <Scintilla.h>
#include "nova_search_bar.h"
#include "nova_script_editor.h"
#include <algorithm>
#include <cstdio>

NovaSearchBar::NovaSearchBar(NovaScriptEditor& editor)
	: _editor(editor)
	, _replace_visible(false)
	, _match_current(0)
	, _match_total(0)
{
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

	this->pack_end(_search_frame, false, false, 12);

	_btn_toggle_replace.signal_clicked().connect(sigc::mem_fun(*this, &NovaSearchBar::toggle_replace_row));
	_btn_find_next.signal_clicked().connect(sigc::mem_fun(*this, &NovaSearchBar::find_next));
	_btn_find_prev.signal_clicked().connect(sigc::mem_fun(*this, &NovaSearchBar::find_prev));
	_btn_replace.signal_clicked().connect(sigc::mem_fun(*this, &NovaSearchBar::do_replace));
	_btn_replace_all.signal_clicked().connect(sigc::mem_fun(*this, &NovaSearchBar::do_replace_all));
	_btn_close_search.signal_clicked().connect(sigc::mem_fun(*this, &NovaSearchBar::hide_bar));

	_search_entry.signal_activate().connect(sigc::mem_fun(*this, &NovaSearchBar::find_next));
	_replace_entry.signal_activate().connect(sigc::mem_fun(*this, &NovaSearchBar::do_replace));
	_search_entry.signal_changed().connect(sigc::mem_fun(*this, &NovaSearchBar::on_text_changed));

	g_signal_connect(_search_entry.gobj(), "key-press-event", G_CALLBACK(&NovaSearchBar::on_entry_key_press), this);
	g_signal_connect(_replace_entry.gobj(), "key-press-event", G_CALLBACK(&NovaSearchBar::on_entry_key_press), this);

	_replace_row.hide();
	_search_frame.show_all();
}

NovaSearchBar::~NovaSearchBar() {}

void
NovaSearchBar::toggle_replace_row()
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
NovaSearchBar::show_bar(bool focus_replace)
{
	this->show();
	_search_frame.show_all();

	if (focus_replace) {
		_replace_visible = true;
		_btn_toggle_replace.set_label(" v ");
		_replace_row.show_all();
	} else if (!_replace_visible) {
		_replace_row.hide();
	}

	intptr_t sel_len = _editor.send_message(SCI_GETSELTEXT, 0, 0);
	if (sel_len > 1) {
		std::string sel(sel_len, '\0');
		_editor.send_message(SCI_GETSELTEXT, 0, (intptr_t)&sel[0]);
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
NovaSearchBar::hide_bar()
{
	_search_frame.hide();
	this->hide();
	_match_current = 0;
	_match_total   = 0;
	_lbl_match_count.set_text("0 of 0");
	_editor.grab_editor_focus();
}

void
NovaSearchBar::on_text_changed()
{
	update_match_count();
}

int
NovaSearchBar::count_all_matches(const std::string& term)
{
	if (term.empty()) return 0;
	const int doc_len = (int)_editor.send_message(SCI_GETLENGTH);
	if (doc_len <= 0) return 0;

	const int old_start = (int)_editor.send_message(SCI_GETSELECTIONSTART);
	const int old_end   = (int)_editor.send_message(SCI_GETSELECTIONEND);

	int count = 0;
	int pos = 0;
	while (pos < doc_len) {
		_editor.send_message(SCI_SETTARGETSTART, pos);
		_editor.send_message(SCI_SETTARGETEND, doc_len);
		_editor.send_message(SCI_SETSEARCHFLAGS, 0);
		int found = (int)_editor.send_message(SCI_SEARCHINTARGET, term.size(), (intptr_t)term.c_str());
		if (found == -1) break;
		++count;
		int t_end = (int)_editor.send_message(SCI_GETTARGETEND);
		if (t_end <= pos) break;
		pos = t_end;
	}

	_editor.send_message(SCI_SETSELECTIONSTART, old_start);
	_editor.send_message(SCI_SETSELECTIONEND, old_end);
	return count;
}

void
NovaSearchBar::update_match_count()
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

	const int cur = (int)_editor.send_message(SCI_GETCURRENTPOS);
	const int doc_len = (int)_editor.send_message(SCI_GETLENGTH);
	int idx = 0;
	int pos = 0;
	int current_idx = 0;

	while (pos < doc_len) {
		_editor.send_message(SCI_SETTARGETSTART, pos);
		_editor.send_message(SCI_SETTARGETEND, doc_len);
		_editor.send_message(SCI_SETSEARCHFLAGS, 0);
		int found = (int)_editor.send_message(SCI_SEARCHINTARGET, term.size(), (intptr_t)term.c_str());
		if (found == -1) break;
		++idx;
		int t_start = (int)_editor.send_message(SCI_GETTARGETSTART);
		int t_end   = (int)_editor.send_message(SCI_GETTARGETEND);
		if (t_start <= cur && cur <= t_end) {
			current_idx = idx;
		} else if (t_start > cur && current_idx == 0) {
			current_idx = idx;
			break;
		}
		if (t_end <= pos) break;
		pos = t_end;
	}

	_match_current = (current_idx == 0) ? 1 : current_idx;

	char buf[64];
	snprintf(buf, sizeof(buf), "%d of %d", _match_current, _match_total);
	_lbl_match_count.set_text(buf);
}

bool
NovaSearchBar::find_with_dir(bool forward)
{
	std::string term = _search_entry.get_text();
	if (term.empty()) return false;

	const int doc_len = (int)_editor.send_message(SCI_GETLENGTH);
	if (doc_len <= 0) return false;

	int from = forward ? (int)_editor.send_message(SCI_GETSELECTIONEND) : (int)_editor.send_message(SCI_GETSELECTIONSTART);
	int found = -1, t_start = -1, t_end = -1;

	if (forward) {
		_editor.send_message(SCI_SETTARGETSTART, from);
		_editor.send_message(SCI_SETTARGETEND, doc_len);
		_editor.send_message(SCI_SETSEARCHFLAGS, 0);
		found = (int)_editor.send_message(SCI_SEARCHINTARGET, term.size(), (intptr_t)term.c_str());
		if (found == -1) {
			_editor.send_message(SCI_SETTARGETSTART, 0);
			_editor.send_message(SCI_SETTARGETEND, doc_len);
			found = (int)_editor.send_message(SCI_SEARCHINTARGET, term.size(), (intptr_t)term.c_str());
		}
		if (found != -1) {
			t_start = (int)_editor.send_message(SCI_GETTARGETSTART);
			t_end   = (int)_editor.send_message(SCI_GETTARGETEND);
		}
	} else {
		int pos = 0, last_start = -1, last_end = -1;
		while (pos < from) {
			_editor.send_message(SCI_SETTARGETSTART, pos);
			_editor.send_message(SCI_SETTARGETEND, from);
			_editor.send_message(SCI_SETSEARCHFLAGS, 0);
			int f = (int)_editor.send_message(SCI_SEARCHINTARGET, term.size(), (intptr_t)term.c_str());
			if (f == -1) break;
			last_start = (int)_editor.send_message(SCI_GETTARGETSTART);
			last_end   = (int)_editor.send_message(SCI_GETTARGETEND);
			if (last_end <= pos) break;
			pos = last_end;
		}
		if (last_start >= 0) {
			t_start = last_start;
			t_end   = last_end;
			found   = last_start;
		} else {
			pos = 0; last_start = -1; last_end = -1;
			while (pos < doc_len) {
				_editor.send_message(SCI_SETTARGETSTART, pos);
				_editor.send_message(SCI_SETTARGETEND, doc_len);
				_editor.send_message(SCI_SETSEARCHFLAGS, 0);
				int f = (int)_editor.send_message(SCI_SEARCHINTARGET, term.size(), (intptr_t)term.c_str());
				if (f == -1) break;
				last_start = (int)_editor.send_message(SCI_GETTARGETSTART);
				last_end   = (int)_editor.send_message(SCI_GETTARGETEND);
				if (last_end <= pos) break;
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

	_editor.send_message(SCI_SETSELECTIONSTART, t_start);
	_editor.send_message(SCI_SETSELECTIONEND, t_end);
	_editor.send_message(SCI_SCROLLCARET);

	_match_total = count_all_matches(term);
	int idx = 0, pos = 0;
	_match_current = 1;
	while (pos < doc_len) {
		_editor.send_message(SCI_SETTARGETSTART, pos);
		_editor.send_message(SCI_SETTARGETEND, doc_len);
		_editor.send_message(SCI_SETSEARCHFLAGS, 0);
		int f = (int)_editor.send_message(SCI_SEARCHINTARGET, term.size(), (intptr_t)term.c_str());
		if (f == -1) break;
		++idx;
		if ((int)_editor.send_message(SCI_GETTARGETSTART) == t_start) {
			_match_current = idx;
			break;
		}
		int te = (int)_editor.send_message(SCI_GETTARGETEND);
		if (te <= pos) break;
		pos = te;
	}

	char buf[64];
	snprintf(buf, sizeof(buf), "%d of %d", _match_current, _match_total);
	_lbl_match_count.set_text(buf);

	_editor.send_message(SCI_SETSELECTIONSTART, t_start);
	_editor.send_message(SCI_SETSELECTIONEND, t_end);
	return true;
}

void NovaSearchBar::find_next() { find_with_dir(true); }
void NovaSearchBar::find_prev() { find_with_dir(false); }

void
NovaSearchBar::do_replace()
{
	std::string term = _search_entry.get_text();
	std::string repl = _replace_entry.get_text();
	if (term.empty()) return;

	intptr_t sel_len = _editor.send_message(SCI_GETSELTEXT, 0, 0);
	if (sel_len > 1) {
		std::string sel(sel_len, '\0');
		_editor.send_message(SCI_GETSELTEXT, 0, (intptr_t)&sel[0]);
		sel.resize(sel_len - 1);

		std::string a = sel, b = term;
		std::transform(a.begin(), a.end(), a.begin(), ::tolower);
		std::transform(b.begin(), b.end(), b.begin(), ::tolower);
		if (a == b) {
			_editor.send_message(SCI_REPLACESEL, 0, (intptr_t)repl.c_str());
		}
	}
	find_next();
}

void
NovaSearchBar::do_replace_all()
{
	std::string term = _search_entry.get_text();
	std::string repl = _replace_entry.get_text();
	if (term.empty()) return;

	const int doc_len = (int)_editor.send_message(SCI_GETLENGTH);
	if (doc_len <= 0) return;

	_editor.send_message(SCI_BEGINUNDOACTION);
	int replacements = 0, pos = 0;
	for (;;) {
		int len_now = (int)_editor.send_message(SCI_GETLENGTH);
		if (pos >= len_now) break;

		_editor.send_message(SCI_SETTARGETSTART, pos);
		_editor.send_message(SCI_SETTARGETEND, len_now);
		_editor.send_message(SCI_SETSEARCHFLAGS, 0);
		int found = (int)_editor.send_message(SCI_SEARCHINTARGET, term.size(), (intptr_t)term.c_str());
		if (found == -1) break;

		int t_start = (int)_editor.send_message(SCI_GETTARGETSTART);
		_editor.send_message(SCI_REPLACETARGET, repl.size(), (intptr_t)repl.c_str());
		++replacements;
		pos = t_start + (int)repl.size();
	}
	_editor.send_message(SCI_ENDUNDOACTION);

	char buf[64];
	snprintf(buf, sizeof(buf), "R: %d", replacements);
	_lbl_match_count.set_text(buf);
	_match_current = 0;
	_match_total   = 0;
}

gboolean
NovaSearchBar::on_entry_key_press(GtkWidget* /*widget*/, GdkEventKey* event, gpointer data)
{
	NovaSearchBar* bar = static_cast<NovaSearchBar*>(data);
	if (!bar || !event) return FALSE;

	if (event->keyval == 0xFF1B) { // Escape
		bar->hide_bar();
		return TRUE;
	}
	if (event->keyval == 0xFFC0) { // Enter
		if (event->state & GDK_SHIFT_MASK) {
			bar->find_prev();
		} else {
			bar->find_next();
		}
		return TRUE;
	}
	return FALSE;
}