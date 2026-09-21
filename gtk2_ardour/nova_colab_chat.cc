#include "nova_colab_chat.h"
#include "nova_colab_network.h"
#include "nova_toast.h"
#include "pbd/i18n.h"
#include <gdk/gdkkeysyms.h>

NovaColabChat::NovaColabChat ()
	: Gtk::VBox (false, 6)
{
	_frame.set_label (_("Session Chat"));
	_frame.set_shadow_type (Gtk::SHADOW_ETCHED_IN);

	Gtk::VBox* inner_vbox = Gtk::manage (new Gtk::VBox (false, 6));
	inner_vbox->set_border_width (6);

	_buffer = Gtk::TextBuffer::create ();
	init_tags ();

	_view.set_buffer (_buffer);
	_view.set_editable (false);
	_view.set_cursor_visible (false);
	_view.set_wrap_mode (Gtk::WRAP_WORD_CHAR);
	
	// Estilo oscuro nativo para el chat
	_view.modify_base (Gtk::STATE_NORMAL, Gdk::Color ("#18181a"));
	_view.modify_text (Gtk::STATE_NORMAL, Gdk::Color ("#e0e0e0"));

	_scroll.set_policy (Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	_scroll.set_shadow_type (Gtk::SHADOW_IN);
	_scroll.set_size_request (-1, 160); // Altura mínima del chat
	_scroll.add (_view);

	_btn_send.set_label (_("Send"));
	_btn_send.signal_clicked ().connect (sigc::mem_fun (*this, &NovaColabChat::on_send_clicked));
	
	_entry.signal_key_press_event ().connect (sigc::mem_fun (*this, &NovaColabChat::on_key_pressed), false);

	_input_box.pack_start (_entry, true, true, 0);
	_input_box.pack_start (_btn_send, false, false, 4);

	inner_vbox->pack_start (_scroll, true, true, 0);
	inner_vbox->pack_start (_input_box, false, false, 0);
	_frame.add (*inner_vbox);

	pack_start (_frame, true, true, 0);
	show_all_children ();
}

NovaColabChat::~NovaColabChat ()
{
}

void
NovaColabChat::init_tags ()
{
	_buffer->create_tag ("system")->property_foreground() = "#aaaaaa";
	
	auto t_me = _buffer->create_tag ("me");
	t_me->property_foreground() = "#00F0FF";
	t_me->property_weight() = Pango::WEIGHT_BOLD;

	auto t_other = _buffer->create_tag ("other");
	t_other->property_foreground() = "#FF5555";
	t_other->property_weight() = Pango::WEIGHT_BOLD;
}

void
NovaColabChat::append_message (const std::string& sender, const std::string& text, bool is_system)
{
	Gtk::TextIter end = _buffer->end ();
	
	std::string tag_name = "other";
	if (is_system) tag_name = "system";
	else if (sender == NovaColabNetwork::instance().my_user_name()) tag_name = "me";

	_buffer->insert_with_tag (_buffer->end (), "[" + sender + "] ", tag_name);
	_buffer->insert (_buffer->end (), text + "\n");

	// Auto-scroll hacia abajo
	_view.scroll_to (_buffer->end ());
}

void
NovaColabChat::clear ()
{
	_buffer->set_text ("");
}

void
NovaColabChat::on_send_clicked ()
{
	std::string text = _entry.get_text ();
	if (text.empty ()) return;

	if (!NovaColabNetwork::instance ().is_connected ()) {
		NovaToast::show_warn (_("Conéctate a una sesión para enviar mensajes"));
		return;
	}

	std::string me = NovaColabNetwork::instance ().my_user_name ();
	append_message (me, text, false);
	NovaColabNetwork::instance ().send_chat (text);
	
	_entry.set_text ("");
	_entry.grab_focus ();
}

bool
NovaColabChat::on_key_pressed (GdkEventKey* ev)
{
	if (ev->keyval == GDK_Return || ev->keyval == GDK_KP_Enter) {
		on_send_clicked ();
		return true;
	}
	return false;
}