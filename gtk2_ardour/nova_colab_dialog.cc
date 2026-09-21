#include "nova_colab_network.h" // PRIMERO: Winsock2 antes de GTK
#include <ytk/ytk.h>
#include "nova_colab_dialog.h"
#include "nova_colab_presence.h"
#include "nova_toast.h"
#include "pbd/i18n.h"
#include <ytkmm/clipboard.h>
#include <sigc++/connection.h>
#include <gdk/gdkkeysyms.h>
#include <cstdlib>
#include <ctime>

NovaColabDialog* NovaColabDialog::_instance = nullptr;

NovaColabDialog::NovaColabDialog ()
	: ArdourDialog (_("NOVA-STUDIO | Live Collaboration"), true, false)
{
	_instance = this;
	set_default_size (480, 520);
	set_position (Gtk::WIN_POS_CENTER);
	set_resizable (true);
	set_keep_above (true);

	build_ui ();
	show_setup_state ();
	show_all_children ();
}

NovaColabDialog::~NovaColabDialog ()
{
	_instance = nullptr;
}

void
NovaColabDialog::toggle_dialog ()
{
	if (_instance) {
		_instance->hide ();
		delete _instance;
		_instance = nullptr;
	} else {
		NovaColabDialog* d = new NovaColabDialog ();
		d->show ();
		d->present ();
	}
}

NovaColabDialog*
NovaColabDialog::instance ()
{
	return _instance;
}

void
NovaColabDialog::build_ui ()
{
	Gtk::VBox* root = get_vbox ();
	root->set_border_width (12);
	root->set_spacing (10);

	build_setup_ui ();
	build_active_ui ();

	root->pack_start (_setup_box, true, true, 0);
	root->pack_start (_active_box, true, true, 0);

	/* Status bar inferior (siempre visible) */
	_lbl_status.set_markup ("<b>● Connection:</b>  Idle");
	_lbl_status.set_alignment (0.5, 0.5);
	Gtk::Frame* status_f = Gtk::manage (new Gtk::Frame ());
	status_f->set_shadow_type (Gtk::SHADOW_IN);
	Gtk::EventBox* status_bg = Gtk::manage (new Gtk::EventBox ());
	status_bg->modify_bg (Gtk::STATE_NORMAL, Gdk::Color ("#1a3a5c"));
	status_bg->add (_lbl_status);
	status_f->add (*status_bg);
	root->pack_start (*status_f, false, false, 4);
}

/* ================================================================
 * ESTADO 1 — SETUP (solo CREATE | JOIN, sin chat)
 * ================================================================ */

void
NovaColabDialog::build_setup_ui ()
{
	_setup_box.set_spacing (10);

	_tabs.set_tab_pos (Gtk::POS_TOP);
	build_create_tab ();
	build_join_tab ();

	_tabs.append_page (_create_box, _("CREATE"));
	_tabs.append_page (_join_box, _("JOIN"));

	_setup_box.pack_start (_tabs, true, true, 0);
}

void
NovaColabDialog::build_create_tab ()
{
	_create_box.set_spacing (12);
	_create_box.set_border_width (12);

	Gtk::HBox* row1 = Gtk::manage (new Gtk::HBox (false, 8));
	Gtk::Label* l1 = Gtk::manage (new Gtk::Label (_("SESSION NAME:")));
	l1->set_alignment (0.0, 0.5);
	l1->set_size_request (120, -1);
	_session_name.set_text ("Tala_Studio_Session_01");
	row1->pack_start (*l1, false, false, 0);
	row1->pack_start (_session_name, true, true, 0);
	_create_box.pack_start (*row1, false, false, 0);

	/* Espaciador */
	_create_box.pack_start (*Gtk::manage (new Gtk::Label ("")), true, true, 0);

	Gtk::HBox* actions = Gtk::manage (new Gtk::HBox (true, 8));
	_btn_start.set_text (_("START SESSION"));
	_btn_start.set_size_request (-1, 38);
	_btn_start.signal_clicked.connect (
		sigc::mem_fun (*this, &NovaColabDialog::on_start_session));

	_btn_cancel_create.set_text (_("CANCEL"));
	_btn_cancel_create.set_size_request (-1, 38);
	_btn_cancel_create.signal_clicked.connect (
		sigc::mem_fun (*this, &NovaColabDialog::on_cancel));

	actions->pack_start (_btn_start, true, true, 0);
	actions->pack_start (_btn_cancel_create, true, true, 0);
	_create_box.pack_start (*actions, false, false, 0);
}

void
NovaColabDialog::build_join_tab ()
{
	_join_box.set_spacing (10);
	_join_box.set_border_width (12);

	Gtk::Label* hint = Gtk::manage (new Gtk::Label ());
	hint->set_markup ("<i>Pega la IP o el link del Host (ej: 127.0.0.1:32550)</i>");
	hint->set_alignment (0.5, 0.5);
	_join_box.pack_start (*hint, false, false, 4);

	Gtk::HBox* row = Gtk::manage (new Gtk::HBox (false, 8));
	Gtk::Label* l = Gtk::manage (new Gtk::Label (_("HOST IP / LINK:")));
	l->set_alignment (0.0, 0.5);
	l->set_size_request (120, -1);
	_join_link.set_text ("127.0.0.1");
	row->pack_start (*l, false, false, 0);
	row->pack_start (_join_link, true, true, 0);
	_join_box.pack_start (*row, false, false, 0);

	_join_box.pack_start (*Gtk::manage (new Gtk::Label ("")), true, true, 0);

	Gtk::HBox* actions = Gtk::manage (new Gtk::HBox (true, 8));
	_btn_join.set_text (_("JOIN SESSION"));
	_btn_join.set_size_request (-1, 38);
	_btn_join.signal_clicked.connect (
		sigc::mem_fun (*this, &NovaColabDialog::on_join_session));

	_btn_cancel_join.set_text (_("CANCEL"));
	_btn_cancel_join.set_size_request (-1, 38);
	_btn_cancel_join.signal_clicked.connect (
		sigc::mem_fun (*this, &NovaColabDialog::on_cancel));

	actions->pack_start (_btn_join, true, true, 0);
	actions->pack_start (_btn_cancel_join, true, true, 0);
	_join_box.pack_start (*actions, false, false, 0);
}

/* ================================================================
 * ESTADO 2 — ACTIVE SESSION (link + users + chat + disconnect)
 * ================================================================ */

void
NovaColabDialog::build_active_ui ()
{
	_active_box.set_spacing (10);

	/* Invite / Share link */
	Gtk::Frame* link_frame = Gtk::manage (new Gtk::Frame (_("Share Invite Link")));
	link_frame->set_shadow_type (Gtk::SHADOW_ETCHED_IN);

	Gtk::HBox* link_row = Gtk::manage (new Gtk::HBox (false, 6));
	link_row->set_border_width (8);

	_invite_entry.set_editable (false);
	_invite_entry.set_text ("");
	link_row->pack_start (_invite_entry, true, true, 0);

	_btn_copy_link.set_text (_("Copy Link"));
	_btn_copy_link.set_size_request (100, -1);
	_btn_copy_link.signal_clicked.connect (
		sigc::mem_fun (*this, &NovaColabDialog::on_copy_link_clicked));
	link_row->pack_start (_btn_copy_link, false, false, 0);

	link_frame->add (*link_row);
	_active_box.pack_start (*link_frame, false, false, 0);

	/* Users online */
	_users_frame.set_label (_("Users Online"));
	_users_frame.set_shadow_type (Gtk::SHADOW_ETCHED_IN);
	_users_vbox.set_spacing (4);
	_users_vbox.set_border_width (6);
	_users_frame.add (_users_vbox);
	_active_box.pack_start (_users_frame, false, false, 0);

	/* Chat */
	build_chat_area ();
	_active_box.pack_start (_chat_frame, true, true, 0);

	/* Disconnect */
	_btn_disconnect.set_text (_("DISCONNECT"));
	_btn_disconnect.set_size_request (-1, 36);
	_btn_disconnect.signal_clicked.connect (
		sigc::mem_fun (*this, &NovaColabDialog::on_disconnect));
	_active_box.pack_start (_btn_disconnect, false, false, 0);
}

void
NovaColabDialog::build_chat_area ()
{
	_chat_frame.set_label (_("Session Chat"));
	_chat_frame.set_shadow_type (Gtk::SHADOW_ETCHED_IN);

	Gtk::VBox* chat_vbox = Gtk::manage (new Gtk::VBox (false, 6));
	chat_vbox->set_border_width (6);

	_chat_buf = Gtk::TextBuffer::create ();
	_chat_view.set_buffer (_chat_buf);
	_chat_view.set_editable (false);
	_chat_view.set_cursor_visible (false);
	_chat_view.set_wrap_mode (Gtk::WRAP_WORD_CHAR);
	_chat_view.modify_base (Gtk::STATE_NORMAL, Gdk::Color ("#141418"));
	_chat_view.modify_text (Gtk::STATE_NORMAL, Gdk::Color ("#E0E0E0"));

	_chat_scroll.set_policy (Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	_chat_scroll.set_shadow_type (Gtk::SHADOW_IN);
	_chat_scroll.set_size_request (-1, 140);
	_chat_scroll.add (_chat_view);

	_btn_send_chat.set_text (_("Send"));
	_btn_send_chat.signal_clicked.connect (
		sigc::mem_fun (*this, &NovaColabDialog::on_send_chat_clicked));

	_chat_entry.signal_key_press_event ().connect (
		sigc::mem_fun (*this, &NovaColabDialog::on_chat_key_pressed), false);

	Gtk::HBox* chat_input_box = Gtk::manage (new Gtk::HBox (false, 4));
	chat_input_box->pack_start (_chat_entry, true, true, 0);
	chat_input_box->pack_start (_btn_send_chat, false, false, 0);

	chat_vbox->pack_start (_chat_scroll, true, true, 0);
	chat_vbox->pack_start (*chat_input_box, false, false, 0);

	_chat_frame.add (*chat_vbox);
}

/* ================================================================
 * Cambio de estado
 * ================================================================ */

void
NovaColabDialog::show_setup_state ()
{
	_active_box.hide ();
	_setup_box.show ();
	_setup_box.show_all_children ();
}

void
NovaColabDialog::show_active_state ()
{
	_setup_box.hide ();
	_active_box.show ();
	_active_box.show_all_children ();
}

/* ================================================================
 * Invite link
 * ================================================================ */

void
NovaColabDialog::generate_invite_token (const std::string& host_hint)
{
	/* Formato simple y testeable: IP:PUERTO */
	if (!host_hint.empty ()) {
		_invite_url = host_hint;
		if (_invite_url.find (':') == std::string::npos) {
			_invite_url += ":32550";
		}
	} else {
		_invite_url = "127.0.0.1:32550";
	}
	_invite_entry.set_text (_invite_url);
}

void
NovaColabDialog::on_copy_link_clicked ()
{
	if (_invite_url.empty ()) {
		NovaToast::show_warn (_("No hay link para copiar"));
		return;
	}

	Glib::RefPtr<Gtk::Clipboard> cb = Gtk::Clipboard::get ();
	cb->set_text (_invite_url);
	NovaToast::show_success (_("Invite link copiado al portapapeles"));
}

/* ================================================================
 * Acciones de sesión
 * ================================================================ */

void
NovaColabDialog::on_start_session ()
{
	clear_users ();

	std::string name = _session_name.get_text ();
	if (name.empty ()) {
		name = "Host (You)";
	} else {
		name = name + " (Host)";
	}

	ColabUser host = { name, "Host", "Edit Lock: Master", "#FF5555", true };
	_users.push_back (host);
	rebuild_users ();

	NovaColabPresence::instance ().clear ();
	bool ok = NovaColabNetwork::instance ().start_server (32550);

	if (ok) {
		generate_invite_token ("127.0.0.1");
		update_status (true, "Active (Port 32550)");
		show_active_state ();
		NovaToast::show_success (_("Session started — copy the invite link"));
		append_chat ("System", "Sesión iniciada. Comparte el link con colaboradores.");
	} else {
		update_status (false, "Failed to bind port 32550");
		NovaToast::show_error (_("No se pudo iniciar el servidor"));
		show_setup_state ();
	}
}

void
NovaColabDialog::on_join_session ()
{
	std::string link = _join_link.get_text ();
	if (link.empty ()) {
		NovaToast::show_warn (_("Escribe IP del Host o el invite link"));
		return;
	}

	/* Permitir "127.0.0.1" o "127.0.0.1:32550" */
	std::string host = link;
	int port = 32550;
	size_t colon = link.find_last_of (':');
	if (colon != std::string::npos) {
		host = link.substr (0, colon);
		port = std::atoi (link.substr (colon + 1).c_str ());
		if (port <= 0) port = 32550;
	}

	clear_users ();
	ColabUser me = { "You (Collaborator)", "Collaborator", "", "#3B82F6", false };
	_users.push_back (me);
	rebuild_users ();

	generate_invite_token (link);

	NovaColabPresence::instance ().clear ();
	NovaColabNetwork::instance ().connect_to_host (host, port);

	update_status (true, "Connecting to " + host + "...");
	show_active_state ();
	NovaToast::show_info (_("Connecting to host..."));
	append_chat ("System", "Conectando a " + host + "...");
}

void
NovaColabDialog::on_cancel ()
{
	hide ();
}

void
NovaColabDialog::on_disconnect ()
{
	clear_users ();
	NovaColabNetwork::instance ().disconnect ();
	NovaColabPresence::instance ().clear ();

	if (_chat_buf) {
		_chat_buf->set_text ("");
	}
	_invite_entry.set_text ("");
	_invite_url.clear ();

	update_status (false, "Idle");
	show_setup_state ();
	NovaToast::show_info (_("Desconectado de la sesión"));
}

/* ================================================================
 * Chat
 * ================================================================ */

void
NovaColabDialog::append_chat (const std::string& who, const std::string& text)
{
	if (!_chat_buf) return;

	Gtk::TextIter start_iter = _chat_buf->end ();
	std::string plain = "[" + who + "] " + text + "\n";
	_chat_buf->insert (start_iter, plain);

	Gtk::TextIter scroll_iter = _chat_buf->end ();
	_chat_view.scroll_to (scroll_iter);
}

void
NovaColabDialog::on_send_chat_clicked ()
{
	std::string text = _chat_entry.get_text ();
	if (text.empty ()) return;

	if (!NovaColabNetwork::instance ().is_connected ()) {
		NovaToast::show_warn (_("No estás conectado a ninguna sesión"));
		return;
	}

	std::string me = NovaColabNetwork::instance ().my_user_name ();
	append_chat (me, text);
	NovaColabNetwork::instance ().send_chat (text);

	_chat_entry.set_text ("");
	_chat_entry.grab_focus ();
}

bool
NovaColabDialog::on_chat_key_pressed (GdkEventKey* ev)
{
	if (ev->keyval == GDK_Return || ev->keyval == GDK_KP_Enter) {
		on_send_chat_clicked ();
		return true;
	}
	return false;
}

/* ================================================================
 * Users list
 * ================================================================ */

void
NovaColabDialog::rebuild_users ()
{
	std::vector<Gtk::Widget*> kids = _users_vbox.get_children ();
	for (Gtk::Widget* k : kids) {
		_users_vbox.remove (*k);
		delete k;
	}
	for (const auto& u : _users) {
		_users_vbox.pack_start (*make_user_row (u), false, false, 0);
	}
	_users_vbox.show_all_children ();
}

Gtk::Widget*
NovaColabDialog::make_user_row (const ColabUser& u)
{
	Gtk::HBox* h = Gtk::manage (new Gtk::HBox (false, 8));
	h->set_border_width (4);

	Gtk::Label* av = Gtk::manage (new Gtk::Label ());
	av->set_markup ("<span size='large' foreground='" + u.color_hex + "'>👤</span>");

	Gtk::VBox* info = Gtk::manage (new Gtk::VBox (false, 0));
	Gtk::Label* n = Gtk::manage (new Gtk::Label ());
	n->set_markup ("<b>" + u.name + "</b>");
	n->set_alignment (0.0, 0.5);
	Gtk::Label* r = Gtk::manage (new Gtk::Label (u.role));
	r->set_alignment (0.0, 0.5);
	info->pack_start (*n, false, false, 0);
	info->pack_start (*r, false, false, 0);

	h->pack_start (*av, false, false, 0);
	h->pack_start (*info, true, true, 0);

	if (!u.lock_status.empty ()) {
		Gtk::Label* lock = Gtk::manage (new Gtk::Label ("🔒 " + u.lock_status));
		h->pack_start (*lock, false, false, 0);
	}

	Gtk::Frame* f = Gtk::manage (new Gtk::Frame ());
	f->set_shadow_type (Gtk::SHADOW_ETCHED_OUT);
	f->add (*h);
	return f;
}

void
NovaColabDialog::update_status (bool connected, const std::string& info)
{
	if (connected) {
		_lbl_status.set_markup (
			"<span foreground='#7DFFB3'><b>● Connection:</b>  "
			+ info + "</span>");
	} else {
		_lbl_status.set_markup (
			"<span foreground='#AAAAAA'><b>● Connection:</b>  "
			+ info + "</span>");
	}
}

void
NovaColabDialog::add_user (const std::string& name, const std::string& role,
                           const std::string& color_hex, bool is_host)
{
	ColabUser u = {
		name,
		role,
		is_host ? "Edit Lock: Master" : "",
		color_hex,
		is_host
	};
	_users.push_back (u);
	rebuild_users ();
	append_chat ("System", name + _(" se unió a la sesión"));
}

void
NovaColabDialog::clear_users ()
{
	_users.clear ();
	std::vector<Gtk::Widget*> kids = _users_vbox.get_children ();
	for (Gtk::Widget* k : kids) {
		_users_vbox.remove (*k);
		delete k;
	}
}