#ifndef __gtk2_ardour_update_checker_h__
#define __gtk2_ardour_update_checker_h__

#include <string>
#include <ytkmm/button.h>
#include <ytkmm/label.h>
#include <ytkmm/progressbar.h>
#include <ytkmm/box.h>
#include <ytkmm/separator.h>
#include "ardour_dialog.h"

class UpdateChecker : public ArdourDialog
{
public:
	UpdateChecker (const std::string& current_version,
	               const std::string& remote_version,
	               const std::string& download_url);
	~UpdateChecker ();

	static void check_and_notify (const std::string& current_version,
	                              const std::string& github_repo);

private:
	std::string _current_version;
	std::string _remote_version;
	std::string _download_url;

	Gtk::Label       _title_label;
	Gtk::Label       _subtitle_label;
	Gtk::Label       _version_box_label;
	Gtk::Label       _info_label;
	Gtk::ProgressBar _progress_bar;
	Gtk::Button      _update_button;
	Gtk::Button      _skip_button;

	void on_update_clicked ();
	void on_skip_clicked ();
	void download_and_install ();
	void set_busy (bool busy);

	static void thread_worker (std::string current_version, std::string github_repo);
	static bool idle_show_dialog (std::string current, std::string remote, std::string url);
	static bool _already_checked;
};

#endif /* __gtk2_ardour_update_checker_h__ */
