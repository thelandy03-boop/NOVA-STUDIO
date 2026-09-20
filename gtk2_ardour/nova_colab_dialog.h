#ifndef __nova_colab_dialog_h__
#define __nova_colab_dialog_h__

#include "ardour_dialog.h"
#include <ytkmm/box.h>
#include <ytkmm/label.h>
#include <ytkmm/button.h>
#include <ytkmm/entry.h>
#include <ytkmm/eventbox.h>
#include <ytkmm/frame.h>
#include <ytkmm/image.h>
#include <ytkmm/notebook.h>
#include <ydkmm/pixbuf.h>
#include <cairo.h>
#include <string>
#include <vector>

struct ColabUser {
	std::string name;
	std::string role;
	std::string lock_status;
	std::string color_hex;
	bool is_host;
};

enum ColabScreen {
	SCREEN_HOME = 0,   /* Join / Create */
	SCREEN_HOST,       /* Host + invite */
	SCREEN_JOIN        /* Join form (stub) */
};

class NovaColabDialog : public ArdourDialog {
public:
	NovaColabDialog();
	~NovaColabDialog();

	static void toggle_dialog();
	static NovaColabDialog* instance();

	void update_status(bool connected, const std::string& server_info);
	void add_user(const std::string& name, const std::string& role, const std::string& color_hex, bool is_host);
	void clear_users();

private:
	static NovaColabDialog* _instance;

	ColabScreen _screen;

	/* --- HOME --- */
	Gtk::VBox   _home_box;
	Gtk::Button _btn_join;
	Gtk::Button _btn_create;

	/* --- HOST / SESSION --- */
	Gtk::VBox     _session_box;
	Gtk::EventBox _status_box;
	Gtk::Label    _lbl_connection_status;
	Gtk::VBox     _users_vbox;
	Gtk::Button   _btn_invite_people;   /* "Invitar gente" */
	Gtk::HBox     _invite_hbox;         /* entry + copy + qr */
	Gtk::Entry    _invite_entry;
	Gtk::Button   _btn_copy_link;
	Gtk::Button   _btn_qr_code;
	Gtk::EventBox _invite_highlight;    /* wrapper para efecto highlight */

	/* --- JOIN (stub) --- */
	Gtk::VBox   _join_box;
	Gtk::Entry  _join_link_entry;
	Gtk::Button _btn_join_confirm;
	Gtk::Button _btn_back_home;

	std::vector<ColabUser> _users;
	bool _invite_highlighted;
	sigc::connection _highlight_timeout;

	void build_ui();
	void show_screen(ColabScreen s);

	void on_create_clicked();
	void on_join_clicked();
	void on_back_home_clicked();
	void on_invite_people_clicked();
	void on_copy_link_clicked();
	void on_join_confirm_clicked();

	void generate_invite_token();
	void rebuild_users_list();
	Gtk::Widget* create_user_row(const ColabUser& user);

	Glib::RefPtr<Gdk::Pixbuf> load_icon_pixbuf(const std::string& name_32,
	                                           const std::string& name_64,
	                                           int target_size);

	bool on_status_pill_expose(GdkEventExpose* event);
	bool on_invite_highlight_expose(GdkEventExpose* event);
	bool clear_invite_highlight();
};

#endif /* __nova_colab_dialog_h__ */