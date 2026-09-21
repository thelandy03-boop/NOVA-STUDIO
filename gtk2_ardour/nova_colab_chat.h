#ifndef __nova_colab_chat_h__
#define __nova_colab_chat_h__

#include <ytkmm/box.h>
#include <ytkmm/button.h>
#include <ytkmm/entry.h>
#include <ytkmm/frame.h>
#include <ytkmm/scrolledwindow.h>
#include <ytkmm/textview.h>
#include <ytkmm/textbuffer.h>
#include <string>

class NovaColabChat : public Gtk::VBox
{
public:
	NovaColabChat ();
	~NovaColabChat ();

	void append_message (const std::string& sender, const std::string& text, bool is_system = false);
	void clear ();

private:
	Gtk::Frame                    _frame;
	Gtk::ScrolledWindow           _scroll;
	Gtk::TextView                 _view;
	Glib::RefPtr<Gtk::TextBuffer> _buffer;

	Gtk::HBox                     _input_box;
	Gtk::Entry                    _entry;
	Gtk::Button                   _btn_send;

	void on_send_clicked ();
	bool on_key_pressed (GdkEventKey* ev);

	void init_tags ();
};

#endif /* __nova_colab_chat_h__ */