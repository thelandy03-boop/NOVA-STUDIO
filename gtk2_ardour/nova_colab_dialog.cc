#include <ytk/ytk.h>
#include "nova_colab_dialog.h"
#include "nova_colab_presence.h"
#include "nova_colab_network.h"
#include "nova_toast.h"
#include "pbd/i18n.h"
#include <ytkmm/clipboard.h>
#include <glibmm/fileutils.h>
#include <glibmm/main.h>
#include <cmath>
#include <cstdlib>
#include <ctime>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

NovaColabDialog* NovaColabDialog::_instance = nullptr;

static void
draw_rounded_rectangle (cairo_t* cr, double x, double y, double w, double h, double r)
{
	cairo_new_sub_path (cr);
	cairo_arc (cr, x + w - r, y + r,     r, -M_PI_2, 0);
	cairo_arc (cr, x + w - r, y + h - r, r, 0,        M_PI_2);
	cairo_arc (cr, x + r,     y + h - r, r, M_PI_2,   M_PI);
	cairo_arc (cr, x + r,     y + r,     r, M_PI,     3 * M_PI_2);
	cairo_close_path (cr);
}

NovaColabDialog::NovaColabDialog ()
	: ArdourDialog (_("NOVA-STUDIO | Live Collaboration"), true, false)
	, _screen (SCREEN_HOME)
	, _invite_highlighted (false)
{
	_instance = this;
	set_default_size (460, 420);
	set_position (Gtk::WIN_POS_CENTER);
	set_resizable (false);
	set_keep_above (true);

	build_ui ();
	show_screen (SCREEN_HOME);
}

NovaColabDialog::~NovaColabDialog ()
{
	if (_highlight_timeout.connected ()) {
		_highlight_timeout.disconnect ();
	}
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
		NovaColabDialog* dlg = new NovaColabDialog ();
		dlg->show ();
		dlg->present ();
	}
}

NovaColabDialog*
NovaColabDialog::instance ()
{
	return _instance;
}

Glib::RefPtr<Gdk::Pixbuf>
NovaColabDialog::load_icon_pixbuf (const std::string& name_32,
                                   const std::string& name_64,
                                   int target_size)
{
	const char* bases[] = {
		"",
		"gtk2_ardour/",
		"gtk2_ardour/icons/",
		"icons/",
		"pixmaps/",
		"../gtk2_ardour/icons/",
		nullptr
	};

	for (int i = 0; bases[i]; ++i) {
		std::string p32 = std::string (bases[i]) + name_32;
		std::string p64 = std::string (bases[i]) + name_64;
		try {
			if (Glib::file_test (p32, Glib::FILE_TEST_EXISTS)) {
				return Gdk::Pixbuf::create_from_file (p32, target_size, target_size);
			}
			if (Glib::file_test (p64, Glib::FILE_TEST_EXISTS)) {
				return Gdk::Pixbuf::create_from_file (p64, target_size, target_size);
			}
		} catch (...) {}
	}
	return Glib::RefPtr<Gdk::Pixbuf> ();
}

void
NovaColabDialog::build_ui ()
{
	Gtk::VBox* main_vbox = get_vbox ();
	main_vbox->set_border_width (16);
	main_vbox->set_spacing (12);

	/* HOME */
	_home_box.set_spacing (16);
	Gtk::Label* home_title = Gtk::manage (new Gtk::Label ());
	home_title->set_markup ("<span weight='bold' size='large' foreground='#FFFFFF'>Live Collaboration</span>");
	home_title->set_alignment (0.5, 0.5);

	Gtk::Label* home_sub = Gtk::manage (new Gtk::Label ());
	home_sub->set_markup ("<span foreground='#AAAAAA'>Elige cómo quieres empezar</span>");
	home_sub->set_alignment (0.5, 0.5);

	_btn_create.set_label (_("  Crear una sesión colaborativa"));
	_btn_create.set_size_request (-1, 48);
	_btn_create.signal_clicked ().connect (sigc::mem_fun (*this, &NovaColabDialog::on_create_clicked));

	_btn_join.set_label (_("  Unirse a una sesión colaborativa"));
	_btn_join.set_size_request (-1, 48);
	_btn_join.signal_clicked ().connect (sigc::mem_fun (*this, &NovaColabDialog::on_join_clicked));

	_home_box.pack_start (*home_title, false, false, 8);
	_home_box.pack_start (*home_sub, false, false, 0);
	_home_box.pack_start (_btn_create, false, false, 8);
	_home_box.pack_start (_btn_join, false, false, 0);

	/* HOST / SESSION */
	_session_box.set_spacing (12);
	_status_box.set_app_paintable (true);
	_status_box.set_size_request (-1, 36);
	_status_box.signal_expose_event ().connect (sigc::mem_fun (*this, &NovaColabDialog::on_status_pill_expose));

	_lbl_connection_status.set_markup ("<span foreground='#E0F2FE' weight='bold'>● WebSocket Connection: [cite: 32550]</span>");
	_lbl_connection_status.set_alignment (0.5, 0.5);
	_status_box.add (_lbl_connection_status);
	_session_box.pack_start (_status_box, false, false, 0);

	Gtk::Label* lbl_users = Gtk::manage (new Gtk::Label ());
	lbl_users->set_markup ("<span weight='bold' foreground='#FFFFFF'>Users Online</span>");
	lbl_users->set_alignment (0.0, 0.5);
	_session_box.pack_start (*lbl_users, false, false, 0);

	_users_vbox.set_spacing (6);
	_session_box.pack_start (_users_vbox, false, false, 0);

	_btn_invite_people.set_label (_("Invitar gente"));
	_btn_invite_people.set_size_request (-1, 36);
	_btn_invite_people.signal_clicked ().connect (sigc::mem_fun (*this, &NovaColabDialog::on_invite_people_clicked));
	_session_box.pack_start (_btn_invite_people, false, false, 4);

	Gtk::Label* lbl_inv = Gtk::manage (new Gtk::Label ());
	lbl_inv->set_markup ("<span weight='bold' foreground='#FFFFFF'>Invite Link</span>");
	lbl_inv->set_alignment (0.0, 0.5);
	_session_box.pack_start (*lbl_inv, false, false, 0);

	_invite_highlight.set_app_paintable (true);
	_invite_highlight.signal_expose_event ().connect (sigc::mem_fun (*this, &NovaColabDialog::on_invite_highlight_expose));

	_invite_hbox.set_spacing (8);
	_invite_hbox.set_border_width (6);

	generate_invite_token ();
	_invite_entry.set_editable (false);
	_invite_entry.modify_base (Gtk::STATE_NORMAL, Gdk::Color ("#181818"));
	_invite_entry.modify_text (Gtk::STATE_NORMAL, Gdk::Color ("#FFFFFF"));

	{
		Glib::RefPtr<Gdk::Pixbuf> link_pix = load_icon_pixbuf ("link32.png", "link64.png", 18);
		Gtk::HBox* box = Gtk::manage (new Gtk::HBox (false, 6));
		if (link_pix) box->pack_start (*Gtk::manage (new Gtk::Image (link_pix)), false, false, 0);
		box->pack_start (*Gtk::manage (new Gtk::Label (_("Copy Link"))), false, false, 0);
		_btn_copy_link.add (*box);
	}
	_btn_copy_link.set_size_request (120, 34);
	_btn_copy_link.signal_clicked ().connect (sigc::mem_fun (*this, &NovaColabDialog::on_copy_link_clicked));

	{
		Glib::RefPtr<Gdk::Pixbuf> qr_pix = load_icon_pixbuf ("qr_code32.png", "qr_code64.png", 22);
		if (qr_pix) _btn_qr_code.add (*Gtk::manage (new Gtk::Image (qr_pix)));
		else _btn_qr_code.set_label (_("QR"));
	}
	_btn_qr_code.set_size_request (40, 34);

	_invite_hbox.pack_start (_invite_entry, true, true, 0);
	_invite_hbox.pack_start (_btn_copy_link, false, false, 0);
	_invite_hbox.pack_start (_btn_qr_code, false, false, 0);

	_invite_highlight.add (_invite_hbox);
	_session_box.pack_start (_invite_highlight, false, false, 0);

	Gtk::Button* btn_back = Gtk::manage (new Gtk::Button (_("← Volver")));
	btn_back->signal_clicked ().connect (sigc::mem_fun (*this, &NovaColabDialog::on_back_home_clicked));
	_session_box.pack_start (*btn_back, false, false, 4);

	/* JOIN SCREEN */
	_join_box.set_spacing (12);
	Gtk::Label* join_title = Gtk::manage (new Gtk::Label ());
	join_title->set_markup ("<span weight='bold' size='large' foreground='#FFFFFF'>Unirse a sesión</span>");
	join_title->set_alignment (0.5, 0.5);

	Gtk::Label* join_hint = Gtk::manage (new Gtk::Label ());
	join_hint->set_markup ("<span foreground='#AAAAAA'>Pega el Invite Link o la IP que te compartieron</span>");
	join_hint->set_alignment (0.5, 0.5);

	_join_link_entry.set_text ("127.0.0.1");
	_btn_join_confirm.set_label (_("Unirse"));
	_btn_join_confirm.set_size_request (-1, 40);
	_btn_join_confirm.signal_clicked ().connect (sigc::mem_fun (*this, &NovaColabDialog::on_join_confirm_clicked));

	_btn_back_home.set_label (_("← Volver"));
	_btn_back_home.signal_clicked ().connect (sigc::mem_fun (*this, &NovaColabDialog::on_back_home_clicked));

	_join_box.pack_start (*join_title, false, false, 8);
	_join_box.pack_start (*join_hint, false, false, 0);
	_join_box.pack_start (_join_link_entry, false, false, 8);
	_join_box.pack_start (_btn_join_confirm, false, false, 0);
	_join_box.pack_start (_btn_back_home, false, false, 8);

	main_vbox->pack_start (_home_box, true, true, 0);
	main_vbox->pack_start (_session_box, true, true, 0);
	main_vbox->pack_start (_join_box, true, true, 0);

	show_all_children ();
}

void
NovaColabDialog::show_screen (ColabScreen s)
{
	_screen = s;
	_home_box.hide ();
	_session_box.hide ();
	_join_box.hide ();

	switch (s) {
	case SCREEN_HOME: _home_box.show (); break;
	case SCREEN_HOST: _session_box.show (); break;
	case SCREEN_JOIN: _join_box.show (); break;
	}
}

void
NovaColabDialog::on_create_clicked ()
{
	clear_users ();
	ColabUser host = {"Host (You)", "Host", "Edit Lock: Master", "#FF5555", true};
	_users.push_back (host);
	rebuild_users_list ();
	generate_invite_token ();
	_invite_highlighted = false;
	_invite_highlight.queue_draw ();
	show_screen (SCREEN_HOST);

	/* Limpiar cursores viejos e Iniciar Servidor Red */
	NovaColabPresence::instance().clear();
	NovaColabNetwork::instance().start_server(32550);
}

void
NovaColabDialog::on_join_clicked ()
{
	show_screen (SCREEN_JOIN);
}

void
NovaColabDialog::on_back_home_clicked ()
{
	clear_users ();
	_invite_highlighted = false;

	NovaColabNetwork::instance().disconnect();
	NovaColabPresence::instance().clear();

	show_screen (SCREEN_HOME);
}

void
NovaColabDialog::on_invite_people_clicked ()
{
	_invite_highlighted = true;
	_invite_highlight.queue_draw ();

	if (_highlight_timeout.connected ()) {
		_highlight_timeout.disconnect ();
	}
	_highlight_timeout = Glib::signal_timeout ().connect (
		sigc::mem_fun (*this, &NovaColabDialog::clear_invite_highlight), 2500);

	NovaToast::show_info (_("Share this link to invite collaborators"));
}

bool
NovaColabDialog::clear_invite_highlight ()
{
	_invite_highlighted = false;
	_invite_highlight.queue_draw ();
	return false;
}

void
NovaColabDialog::on_copy_link_clicked ()
{
	Gtk::Clipboard::get ()->set_text (_invite_entry.get_text ());
	NovaToast::show_success (_("Invite link copied to clipboard"));
}

void
NovaColabDialog::on_join_confirm_clicked ()
{
	std::string link = _join_link_entry.get_text ();
	if (link.empty ()) return;

	clear_users ();
	ColabUser me = { "You (Collaborator)", "Collaborator", "", "#3B82F6", false };
	_users.push_back (me);
	rebuild_users_list ();
	_invite_entry.set_text (link);
	show_screen (SCREEN_HOST);

	/* Limpiar cursores viejos e Iniciar Conexión Red Real */
	NovaColabPresence::instance().clear();
	NovaColabNetwork::instance().connect_to_host(link, 32550);
}

void
NovaColabDialog::rebuild_users_list ()
{
	std::vector<Gtk::Widget*> kids = _users_vbox.get_children ();
	for (Gtk::Widget* k : kids) {
		_users_vbox.remove (*k);
		delete k;
	}
	for (const auto& u : _users) {
		_users_vbox.pack_start (*create_user_row (u), false, false, 0);
	}
	_users_vbox.show_all_children ();
}

Gtk::Widget*
NovaColabDialog::create_user_row (const ColabUser& user)
{
	Gtk::Frame* frame = Gtk::manage (new Gtk::Frame ());
	frame->set_shadow_type (Gtk::SHADOW_ETCHED_OUT);

	Gtk::EventBox* card_bg = Gtk::manage (new Gtk::EventBox ());
	card_bg->modify_bg (Gtk::STATE_NORMAL, Gdk::Color ("#222222"));

	Gtk::HBox* hbox = Gtk::manage (new Gtk::HBox (false, 12));
	hbox->set_border_width (10);

	Gtk::Label* avatar = Gtk::manage (new Gtk::Label ());
	avatar->set_markup ("<span size='xx-large' foreground='" + user.color_hex + "'>👤</span>");

	Gtk::VBox* info = Gtk::manage (new Gtk::VBox (false, 2));
	Gtk::Label* name = Gtk::manage (new Gtk::Label ());
	name->set_markup ("<span weight='bold' foreground='#FFFFFF'>" + user.name + "</span>");
	name->set_alignment (0.0, 0.5);
	Gtk::Label* role = Gtk::manage (new Gtk::Label ());
	role->set_markup ("<span size='small' foreground='#AAAAAA'>" + user.role + "</span>");
	role->set_alignment (0.0, 0.5);
	info->pack_start (*name, false, false, 0);
	info->pack_start (*role, false, false, 0);

	hbox->pack_start (*avatar, false, false, 0);
	hbox->pack_start (*info, true, true, 0);

	if (!user.lock_status.empty ()) {
		Gtk::Label* lock = Gtk::manage (new Gtk::Label ());
		lock->set_markup ("<span size='small' weight='bold' foreground='#E0E0E0'>🔒 " + user.lock_status + "</span>");
		lock->set_alignment (1.0, 0.5);
		hbox->pack_start (*lock, false, false, 0);
	}

	card_bg->add (*hbox);
	frame->add (*card_bg);
	return frame;
}

bool
NovaColabDialog::on_status_pill_expose (GdkEventExpose*)
{
	Glib::RefPtr<Gdk::Window> win = _status_box.get_window ();
	if (!win) return false;

	cairo_t* cr = gdk_cairo_create (win->gobj ());
	int w = _status_box.get_allocation ().get_width ();
	int h = _status_box.get_allocation ().get_height ();

	draw_rounded_rectangle (cr, 1, 1, w - 2, h - 2, (h - 2) / 2.0);

	cairo_pattern_t* pat = cairo_pattern_create_linear (0, 0, w, 0);
	cairo_pattern_add_color_stop_rgb (pat, 0.0, 0.16, 0.44, 0.80);
	cairo_pattern_add_color_stop_rgb (pat, 1.0, 0.10, 0.30, 0.58);
	cairo_set_source (cr, pat);
	cairo_fill_preserve (cr);
	cairo_pattern_destroy (pat);

	cairo_set_source_rgba (cr, 0.45, 0.75, 1.0, 0.4);
	cairo_set_line_width (cr, 1.2);
	cairo_stroke (cr);

	cairo_set_source_rgba (cr, 0.65, 0.88, 1.0, 0.85);
	cairo_set_line_width (cr, 2.0);
	cairo_arc (cr, w - 22, h / 2.0, 5, -M_PI / 3.0, M_PI / 3.0);
	cairo_stroke (cr);
	cairo_arc (cr, w - 17, h / 2.0, 9, -M_PI / 3.0, M_PI / 3.0);
	cairo_stroke (cr);

	cairo_destroy (cr);
	return false;
}

bool
NovaColabDialog::on_invite_highlight_expose (GdkEventExpose*)
{
	if (!_invite_highlighted) return false;

	Glib::RefPtr<Gdk::Window> win = _invite_highlight.get_window ();
	if (!win) return false;

	cairo_t* cr = gdk_cairo_create (win->gobj ());
	int w = _invite_highlight.get_allocation ().get_width ();
	int h = _invite_highlight.get_allocation ().get_height ();

	draw_rounded_rectangle (cr, 1, 1, w - 2, h - 2, 8.0);
	cairo_set_source_rgba (cr, 0.2, 0.7, 1.0, 0.25);
	cairo_fill_preserve (cr);
	cairo_set_source_rgba (cr, 0.3, 0.85, 1.0, 0.9);
	cairo_set_line_width (cr, 2.0);
	cairo_stroke (cr);

	cairo_destroy (cr);
	return false;
}

void
NovaColabDialog::generate_invite_token ()
{
	static const char alphanum[] = "0123456789abcdef";
	std::string token;
	srand (static_cast<unsigned> (time (nullptr)));
	for (int i = 0; i < 8; ++i) {
		token += alphanum[rand () % (sizeof (alphanum) - 1)];
	}
	_invite_entry.set_text ("https://nova.io/" + token);
}

void
NovaColabDialog::update_status (bool connected, const std::string& server_info)
{
	if (connected) {
		_lbl_connection_status.set_markup ("<span foreground='#E0F2FE' weight='bold'>● WebSocket Connection: [" + server_info + "]</span>");
	} else {
		_lbl_connection_status.set_markup ("<span foreground='#FF4D4D' weight='bold'>● Disconnected</span>");
	}
}

void
NovaColabDialog::add_user (const std::string& name, const std::string& role, const std::string& color_hex, bool is_host)
{
	ColabUser u = { name, role, is_host ? "Edit Lock: Master" : "", color_hex, is_host };
	_users.push_back (u);
	rebuild_users_list ();
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