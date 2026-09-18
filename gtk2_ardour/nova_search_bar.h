#ifndef __nova_search_bar_h__
#define __nova_search_bar_h__

#include <ytkmm/box.h>
#include <ytkmm/entry.h>
#include <ytkmm/label.h>
#include <ytkmm/button.h>
#include <ytkmm/frame.h>
#include <string>

class NovaScriptEditor;

class NovaSearchBar : public Gtk::HBox {
public:
	NovaSearchBar(NovaScriptEditor& editor);
	~NovaSearchBar();

	void show_bar(bool focus_replace = false);
	void hide_bar();
	void update_match_count();

	bool handle_key_escape();
	void find_next();
	void find_prev();

private:
	NovaScriptEditor& _editor;

	Gtk::Frame  _search_frame;
	Gtk::VBox   _search_panel;
	Gtk::HBox   _find_row;
	Gtk::HBox   _replace_row;

	Gtk::Button _btn_toggle_replace;
	Gtk::Entry  _search_entry;
	Gtk::Label  _lbl_match_count;
	Gtk::Button _btn_find_prev;
	Gtk::Button _btn_find_next;
	Gtk::Button _btn_close_search;

	Gtk::Entry  _replace_entry;
	Gtk::Button _btn_replace;
	Gtk::Button _btn_replace_all;

	bool _replace_visible;
	int  _match_current;
	int  _match_total;

	void toggle_replace_row();
	void do_replace();
	void do_replace_all();
	void on_text_changed();
	
	int  count_all_matches(const std::string& term);
	bool find_with_dir(bool forward);

	static gboolean on_entry_key_press(GtkWidget* widget, GdkEventKey* event, gpointer data);
};

#endif /* __nova_search_bar_h__ */