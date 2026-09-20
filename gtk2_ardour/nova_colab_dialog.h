#ifndef __nova_colab_dialog_h__
#define __nova_colab_dialog_h__

#include "ardour_dialog.h"

#include <ytkmm/box.h>
#include <ytkmm/label.h>
#include <ytkmm/button.h>
#include <ytkmm/entry.h>
#include <ytkmm/notebook.h>
#include <ytkmm/frame.h>
#include <string>
#include <vector>

struct ColabUser {
	std::string name;
	std::string role;
	std::string lock_status;
	std::string color_hex;
	bool is_host;
};

class NovaColabDialog : public ArdourDialog {
public:
	NovaColabDialog ();
	~NovaColabDialog ();

	static void toggle_dialog ();
	static NovaColabDialog* instance ();

	void update_status (bool connected, const std::string& server_info);
	void add_user (const std::string& name, const std::string& role,
	               const std::string& color_hex, bool is_host);
	void clear_users ();

private:
	static NovaColabDialog* _instance;

	Gtk::Notebook _tabs;

	/* --- CREATE tab --- */
	Gtk::VBox          _create_box;
	Gtk::Entry         _session_name;
	Gtk::Button        _btn_start;
	Gtk::Button        _btn_cancel_create;

	/* --- JOIN tab --- */
	Gtk::VBox          _join_box;
	Gtk::Entry         _join_link;
	Gtk::Button        _btn_join;
	Gtk::Button        _btn_cancel_join;

	/* --- shared --- */
	Gtk::Label         _lbl_status;
	Gtk::VBox          _users_vbox;
	Gtk::Frame         _users_frame;

	std::vector<ColabUser> _users;

	void build_ui ();
	void build_create_tab ();
	void build_join_tab ();

	void on_start_session ();
	void on_join_session ();
	void on_cancel ();

	void rebuild_users ();
	Gtk::Widget* make_user_row (const ColabUser& u);
};

#endif /* __nova_colab_dialog_h__ */