#ifndef __nova_colab_dialog_h__
#define __nova_colab_dialog_h__

#include "ardour_dialog.h"
#include "widgets/ardour_button.h"

#include <ytkmm/box.h>
#include <ytkmm/label.h>
#include <ytkmm/entry.h>
#include <ytkmm/eventbox.h>
#include <ytkmm/frame.h>
#include <ytkmm/scrolledwindow.h>
#include <ytkmm/textview.h>
#include <ytkmm/textbuffer.h>
#include <ytkmm/notebook.h>
#include <gdk/gdk.h>
#include <string>
#include <vector>

using ArdourWidgets::ArdourButton;

struct ColabUser {
	std::string name;
	std::string role;
	std::string lock_status;
	std::string color_hex;
	bool is_host;
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

	void append_chat(const std::string& who, const std::string& text);

private:
	static NovaColabDialog* _instance;

	/* Contenedores de estado */
	Gtk::VBox _setup_box;
	Gtk::VBox _active_box;

	/* --- SETUP: CREATE | JOIN --- */
	Gtk::Notebook _tabs;

	Gtk::VBox     _create_box;
	Gtk::Entry    _session_name;
	ArdourButton  _btn_start;
	ArdourButton  _btn_cancel_create;

	Gtk::VBox     _join_box;
	Gtk::Entry    _join_link;
	ArdourButton  _btn_join;
	ArdourButton  _btn_cancel_join;

	/* --- ACTIVE SESSION --- */
	Gtk::Entry    _invite_entry;
	ArdourButton  _btn_copy_link;

	Gtk::Frame    _users_frame;
	Gtk::VBox     _users_vbox;

	Gtk::Frame                    _chat_frame;
	Gtk::ScrolledWindow           _chat_scroll;
	Gtk::TextView                 _chat_view;
	Glib::RefPtr<Gtk::TextBuffer> _chat_buf;
	Gtk::Entry                    _chat_entry;
	ArdourButton                  _btn_send_chat;

	ArdourButton  _btn_disconnect;

	/* Status siempre visible */
	Gtk::Label    _lbl_status;

	std::vector<ColabUser> _users;
	std::string            _invite_url;

	void build_ui();
	void build_setup_ui();
	void build_create_tab();
	void build_join_tab();
	void build_active_ui();
	void build_chat_area();

	void show_setup_state();
	void show_active_state();

	void on_start_session();
	void on_join_session();
	void on_cancel();
	void on_disconnect();
	void on_copy_link_clicked();

	void on_send_chat_clicked();
	bool on_chat_key_pressed(GdkEventKey* ev);

	void rebuild_users();
	Gtk::Widget* make_user_row(const ColabUser& user);

	void generate_invite_token(const std::string& host_hint);
};

#endif /* __nova_colab_dialog_h__ */