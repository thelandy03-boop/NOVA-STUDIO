#include "update_checker.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <unistd.h>

#include <glibmm/main.h>
#include <ytk/ytk.h>

#include "pbd/i18n.h"

using namespace Gtk;
using namespace std;

bool UpdateChecker::_already_checked = false;

static string
strip_v (string v)
{
	if (!v.empty () && (v[0] == 'v' || v[0] == 'V')) {
		return v.substr (1);
	}
	return v;
}

UpdateChecker::UpdateChecker (const string& current_version,
                              const string& remote_version,
                              const string& download_url)
	: ArdourDialog (_("NOVA-STUDIO — Actualización disponible"), true, false)
	, _current_version (strip_v (current_version))
	, _remote_version (strip_v (remote_version))
	, _download_url (download_url)
	, _update_button (_("Actualizar ahora"))
	, _skip_button (_("Más tarde"))
{
	set_default_size (450, 250);
	set_resizable (false);
	set_border_width (0);
	set_position (WIN_POS_CENTER);

	VBox* main_vbox = get_vbox ();
	main_vbox->set_spacing (0);

	/* --- Cabecera con título e icono --- */
	VBox* header_box = manage (new VBox ());
	header_box->set_border_width (16);
	header_box->set_spacing (4);

	_title_label.set_markup ("<span size=\"large\" weight=\"bold\" foreground=\"#FFFFFF\">NOVA-STUDIO</span>");
	_title_label.set_alignment (0.0, 0.5);

	_subtitle_label.set_markup ("<span size=\"medium\" foreground=\"#CCCCCC\">Una nueva versión está lista para instalar</span>");
	_subtitle_label.set_alignment (0.0, 0.5);

	header_box->pack_start (_title_label, false, false, 0);
	header_box->pack_start (_subtitle_label, false, false, 0);
	main_vbox->pack_start (*header_box, false, false, 0);

	main_vbox->pack_start (*manage (new HSeparator ()), false, false, 0);

	/* --- Cuerpo: Comparativa de versiones --- */
	VBox* body_box = manage (new VBox ());
	body_box->set_border_width (16);
	body_box->set_spacing (12);

	HBox* versions_hbox = manage (new HBox (true, 16));

	// Tarjeta Versión Actual
	VBox* cur_card = manage (new VBox ());
	Label* cur_title = manage (new Label ());
	cur_title->set_markup ("<span size=\"small\" weight=\"bold\" foreground=\"#888888\">INSTALADA</span>");
	cur_title->set_alignment (0.5, 0.5);
	Label* cur_val = manage (new Label ());
	cur_val->set_markup ("<span size=\"x-large\" weight=\"bold\" foreground=\"#FF6B6B\">" + _current_version + "</span>");
	cur_val->set_alignment (0.5, 0.5);
	cur_card->pack_start (*cur_title, false, false, 0);
	cur_card->pack_start (*cur_val, false, false, 4);

	// Tarjeta Nueva Versión
	VBox* rem_card = manage (new VBox ());
	Label* rem_title = manage (new Label ());
	rem_title->set_markup ("<span size=\"small\" weight=\"bold\" foreground=\"#888888\">NUEVA</span>");
	rem_title->set_alignment (0.5, 0.5);
	Label* rem_val = manage (new Label ());
	rem_val->set_markup ("<span size=\"x-large\" weight=\"bold\" foreground=\"#51CF66\">" + _remote_version + "</span>");
	rem_val->set_alignment (0.5, 0.5);
	rem_card->pack_start (*rem_title, false, false, 0);
	rem_card->pack_start (*rem_val, false, false, 4);

	versions_hbox->pack_start (*cur_card);
	versions_hbox->pack_start (*rem_card);
	body_box->pack_start (*versions_hbox, false, false, 0);

	_info_label.set_markup ("<span size=\"small\" foreground=\"#AAAAAA\">Se descargarán los nuevos binarios compilados y se instalarán automáticamente en el sistema.</span>");
	_info_label.set_alignment (0.5, 0.5);
	_info_label.set_line_wrap (true);
	body_box->pack_start (_info_label, false, false, 0);

	/* Barra de progreso */
	_progress_bar.set_text (_("Preparando descarga..."));
	_progress_bar.set_fraction (0.0);
	_progress_bar.set_no_show_all (true);
	_progress_bar.hide ();
	body_box->pack_start (_progress_bar, false, false, 0);

	main_vbox->pack_start (*body_box, true, true, 0);

	main_vbox->pack_start (*manage (new HSeparator ()), false, false, 0);

	/* --- Botones --- */
	HBox* action_box = manage (new HBox (false, 12));
	action_box->set_border_width (12);

	_skip_button.set_size_request (110, 32);
	_update_button.set_size_request (150, 32);
	_update_button.set_can_default (true);

	action_box->pack_end (_update_button, false, false, 0);
	action_box->pack_end (_skip_button, false, false, 0);
	main_vbox->pack_start (*action_box, false, false, 0);

	_update_button.signal_clicked ().connect (sigc::mem_fun (*this, &UpdateChecker::on_update_clicked));
	_skip_button.signal_clicked ().connect (sigc::mem_fun (*this, &UpdateChecker::on_skip_clicked));

	show_all_children ();
	_progress_bar.hide ();
}

UpdateChecker::~UpdateChecker () {}

void
UpdateChecker::set_busy (bool busy)
{
	_update_button.set_sensitive (!busy);
	_skip_button.set_sensitive (!busy);
}

void
UpdateChecker::on_update_clicked ()
{
	set_busy (true);
	_progress_bar.show ();
	_progress_bar.set_fraction (0.1);
	_progress_bar.set_text (_("Conectando con GitHub..."));

	while (gtk_events_pending ()) {
		gtk_main_iteration ();
	}

	download_and_install ();
}

void
UpdateChecker::on_skip_clicked ()
{
	response (RESPONSE_CANCEL);
}

void
UpdateChecker::download_and_install ()
{
	_progress_bar.set_fraction (0.3);
	_progress_bar.set_text (_("Descargando actualización..."));
	while (gtk_events_pending ()) {
		gtk_main_iteration ();
	}

	if (_download_url.empty ()) {
		_progress_bar.set_text (_("Error: No se encontró URL de descarga"));
		_skip_button.set_label (_("Cerrar"));
		_skip_button.set_sensitive (true);
		return;
	}

	string tmp_dir = "/tmp/nova_upgrade_" + to_string (getpid ());
	string tarball = tmp_dir + "/update.tar.gz";
	string dl_cmd  = "mkdir -p '" + tmp_dir + "' && curl -fL -o '" + tarball + "' '" + _download_url + "' 2>/dev/null";

	int dl_ret = system (dl_cmd.c_str ());
	if (dl_ret != 0) {
		_progress_bar.set_fraction (0.0);
		_progress_bar.set_text (_("Error en la descarga"));
		_skip_button.set_label (_("Cerrar"));
		_skip_button.set_sensitive (true);
		return;
	}

	_progress_bar.set_fraction (0.7);
	_progress_bar.set_text (_("Instalando binarios en /usr/local..."));
	while (gtk_events_pending ()) {
		gtk_main_iteration ();
	}

	string install_cmd = "sudo -n tar -xzf '" + tarball + "' -C /usr/local/ 2>/dev/null || sudo tar -xzf '" + tarball + "' -C /usr/local/ 2>/dev/null";
	int inst_ret = system (install_cmd.c_str ());

	system (("rm -rf '" + tmp_dir + "'").c_str ());

	_progress_bar.set_fraction (1.0);
	if (inst_ret == 0) {
		_progress_bar.set_text (_("¡Actualización completada!"));
		_info_label.set_markup ("<span size=\"small\" foreground=\"#51CF66\">Reinicia NOVA-STUDIO para aplicar los cambios.</span>");

		string version_file = string (getenv ("HOME") ? getenv ("HOME") : ".") + "/.nova_studio_version";
		ofstream vf (version_file.c_str ());
		if (vf) vf << _remote_version << "\n";
	} else {
		_progress_bar.set_text (_("Fallo al escribir en /usr/local"));
	}

	_skip_button.set_label (_("Cerrar"));
	_skip_button.set_sensitive (true);
}

void
UpdateChecker::check_and_notify (const string& current_version, const string& github_repo)
{
	if (_already_checked) {
		return;
	}
	_already_checked = true;

	std::thread (thread_worker, current_version, github_repo).detach ();
}

void
UpdateChecker::thread_worker (string current_version, string github_repo)
{
	sleep (2);

	string cmd = "curl -s --max-time 4 \"https://api.github.com/repos/" + github_repo + "/releases/latest\" 2>/dev/null";

	FILE* pipe = popen (cmd.c_str (), "r");
	if (!pipe) return;

	char buffer[2048];
	string response_json = "";
	while (fgets (buffer, sizeof (buffer), pipe) != NULL) {
		response_json += buffer;
	}
	pclose (pipe);

	if (response_json.empty ()) return;

	size_t tag_pos = response_json.find ("\"tag_name\"");
	if (tag_pos == string::npos) return;

	size_t first_quote  = response_json.find ("\"", tag_pos + 10);
	size_t second_quote = response_json.find ("\"", first_quote + 1);
	if (first_quote == string::npos || second_quote == string::npos) return;

	string remote_version = response_json.substr (first_quote + 1, second_quote - first_quote - 1);

	string v_rem = strip_v (remote_version);
	string v_loc = strip_v (current_version);

	if (v_rem.empty () || v_rem == v_loc) return;

	size_t url_pos = response_json.find (".tar.gz");
	if (url_pos == string::npos) return;

	size_t url_start = response_json.rfind ("\"browser_download_url\"", url_pos);
	if (url_start == string::npos) return;

	size_t url_val_start = response_json.find ("\"", url_start + 22);
	size_t url_val_end   = response_json.find ("\"", url_val_start + 1);
	if (url_val_start == string::npos || url_val_end == string::npos) return;

	string download_url = response_json.substr (url_val_start + 1, url_val_end - url_val_start - 1);

	Glib::signal_idle ().connect (
	    sigc::bind (sigc::ptr_fun (&UpdateChecker::idle_show_dialog),
	                current_version, remote_version, download_url));
}

bool
UpdateChecker::idle_show_dialog (string current, string remote, string url)
{
	UpdateChecker* dialog = new UpdateChecker (current, remote, url);
	dialog->run ();
	delete dialog;
	return false;
}
