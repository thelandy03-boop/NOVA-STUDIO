#pragma once
#ifndef GTK
#define GTK
#endif

#include <ytkmm/box.h>
#include <string>

extern "C" {
#include "scintilla/include/Scintilla.h"
#include "scintilla/include/SciLexer.h"
#include "scintilla/include/ScintillaWidget.h"
}

class NovaScriptEditor : public Gtk::VBox
{
public:
	NovaScriptEditor();
	virtual ~NovaScriptEditor();

	void set_text (const std::string& text);
	std::string get_text () const;

	void set_lua_syntax ();
	void set_dark_cyberpunk_theme ();

	void setup_autocompletion ();
	void show_autocompletion (const std::string& word_list);

	void get_cursor_position (int& line, int& col) const;

	sigc::signal<void>& signal_text_changed() { return _signal_text_changed; }
	sigc::signal<void>& signal_cursor_changed() { return _signal_cursor_changed; }

private:
	ScintillaObject* _sci;
	sigc::signal<void> _signal_text_changed;
	sigc::signal<void> _signal_cursor_changed;

	intptr_t send_sci (unsigned int msg, uintptr_t wp = 0, intptr_t lp = 0) const;
	static void on_notify (GtkWidget* w, gint param, SCNotification* n, gpointer data);
};