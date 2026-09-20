#include <ytk/ytk.h>
#include "nova_colab_dialog.h"
#include "nova_colab_presence.h"
#include "nova_colab_network.h"
#include "nova_toast.h"
#include "pbd/i18n.h"
#include <ytkmm/clipboard.h>
#include <cstdlib>
#include <ctime>

NovaColabDialog* NovaColabDialog::_instance = nullptr;

NovaColabDialog::NovaColabDialog ()
	: ArdourDialog (_("NOVA-STUDIO | Live Collaboration"), true, false)
{
	_instance = this;
	set_default_size (440, 340);
	set_position (Gtk::WIN_POS_CENTER);
	set_resizable (false);
	set_keep_above (true);

	build_ui ();
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

	/* Notebook: CREATE | JOIN */
	_tabs.set_tab_pos (Gtk::POS_TOP);
	build_create_tab ();
	build_join_tab ();

	_tabs.append_page (_create_box, _("CREATE"));
	_tabs.append_page (_join_box, _("JOIN"));

	root->pack_start (_tabs, true, true, 0);

	/* Users online list */
	_users_frame.set_label (_("Users Online"));
	_users_frame.set_shadow_type (Gtk::SHADOW_ETCHED_IN);
	_users_vbox.set_spacing (4);
	_users_vbox.set_border_width (6);
	_users_frame.add (_users_vbox);
	root->pack_start (_users_frame, false, false, 0);

	/* Status bar inferior */
	_lbl_status.set_markup ("<b>● WebSocket Connection:</b>  Idle");
	_lbl_status.set_alignment (0.5, 0.5);
	Gtk::Frame* status_f = Gtk::manage (new Gtk::Frame ());
	status_f->set_shadow_type (Gtk::SHADOW_IN);
	Gtk::EventBox* status_bg = Gtk::manage (new Gtk::EventBox ());
	status_bg->modify_bg (Gtk::STATE_NORMAL, Gdk::Color ("#1a3a5c"));
	status_bg->add (_lbl_status);
	status_f->add (*status_bg);
	root->pack_start (*status_f, false, false, 4);
}

void
NovaColabDialog::build_create_tab ()
{
	_create_box.set_spacing (12);
	_create_box.set_border_width (12);

	/* SESSION NAME */
	Gtk::HBox* row1 = Gtk::manage (new Gtk::HBox (false, 8));
	Gtk::Label* l1 = Gtk::manage (new Gtk::Label (_("SESSION NAME:")));
	l1->set_alignment (0.0, 0.5);
	l1->set_size_request (110, -1);
	_session_name.set_text ("Tala_Studio_Session_01");
	row1->pack_start (*l1, false, false, 0);
	row1->pack_start (_session_name, true, true, 0);
	_create_box.pack_start (*row1, false, false, 0);

	/* spacer */
	_create_box.pack_start (*Gtk::manage (new Gtk::Label ("")), true, true, 0);

	/* START / CANCEL */
	Gtk::HBox* actions = Gtk::manage (new Gtk::HBox (true, 8));
	_btn_start.set_label (_("START SESSION"));
	_btn_start.set_size_request (-1, 38);
	_btn_start.signal_clicked ().connect (
		sigc::mem_fun (*this, &NovaColabDialog::on_start_session));
	_btn_cancel_create.set_label (_("CANCEL"));
	_btn_cancel_create.set_size_request (-1, 38);
	_btn_cancel_create.signal_clicked ().connect (
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
	hint->set_markup ("<i>Pega la IP del Host (ej: 127.0.0.1)</i>");
	hint->set_alignment (0.5, 0.5);
	_join_box.pack_start (*hint, false, false, 4);

	Gtk::HBox* row = Gtk::manage (new Gtk::HBox (false, 8));
	Gtk::Label* l = Gtk::manage (new Gtk::Label (_("HOST IP / LINK:")));
	l->set_alignment (0.0, 0.5);
	l->set_size_request (110, -1);
	_join_link.set_text ("127.0.0.1");
	row->pack_start (*l, false, false, 0);
	row->pack_start (_join_link, true, true, 0);
	_join_box.pack_start (*row, false, false, 0);

	_join_box.pack_start (*Gtk::manage (new Gtk::Label ("")), true, true, 0);

	Gtk::HBox* actions = Gtk::manage (new Gtk::HBox (true, 8));
	_btn_join.set_label (_("JOIN SESSION"));
	_btn_join.set_size_request (-1, 38);
	_btn_join.signal_clicked ().connect (
		sigc::mem_fun (*this, &NovaColabDialog::on_join_session));
	_btn_cancel_join.set_label (_("CANCEL"));
	_btn_cancel_join.set_size_request (-1, 38);
	_btn_cancel_join.signal_clicked ().connect (
		sigc::mem_fun (*this, &NovaColabDialog::on_cancel));
	actions->pack_start (_btn_join, true, true, 0);
	actions->pack_start (_btn_cancel_join, true, true, 0);
	_join_box.pack_start (*actions, false, false, 0);
}

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
		update_status (true, "Connected (ACTIVE)  ·  32550");
		NovaToast::show_success (_("Session started — you are the Host"));
	} else {
		update_status (false, "Failed to bind port 32550");
		NovaToast::show_error (_("No se pudo iniciar el servidor"));
	}
}

void
NovaColabDialog::on_join_session ()
{
	std::string link = _join_link.get_text ();
	if (link.empty ()) {
		NovaToast::show_warn (_("Escribe IP del Host"));
		return;
	}

	clear_users ();
	ColabUser me = { "You (Collaborator)", "Collaborator", "", "#3B82F6", false };
	_users.push_back (me);
	rebuild_users ();

	NovaColabPresence::instance ().clear ();
	NovaColabNetwork::instance ().connect_to_host (link, 32550);

	update_status (true, "Connecting to " + link + "...");
	NovaToast::show_info (_("Connecting to host..."));
}

void
NovaColabDialog::on_cancel ()
{
	clear_users ();
	NovaColabNetwork::instance ().disconnect ();
	NovaColabPresence::instance ().clear ();
	update_status (false, "Idle");
	hide ();
}

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
			"<span foreground='#7DFFB3'><b>● WebSocket Connection:</b>  "
			+ info + "</span>");
	} else {
		_lbl_status.set_markup (
			"<span foreground='#AAAAAA'><b>● WebSocket Connection:</b>  "
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