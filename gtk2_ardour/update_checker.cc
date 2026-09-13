#include "update_checker.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>

#include <glibmm/main.h>
#include <ytk/ytk.h>

#include "pbd/i18n.h"

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#endif

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
	set_keep_above (true);

	VBox* main_vbox = get_vbox ();
	main_vbox->set_spacing (0);

	/* --- Cabecera --- */
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

	/* --- Cuerpo: Comparativa --- */
	VBox* body_box = manage (new VBox ());
	body_box->set_border_width (16);
	body_box->set_spacing (12);

	HBox* versions_hbox = manage (new HBox (true, 16));

	VBox* cur_card = manage (new VBox ());
	Label* cur_title = manage (new Label ());
	cur_title->set_markup ("<span size=\"small\" weight=\"bold\" foreground=\"#888888\">INSTALADA</span>");
	cur_title->set_alignment (0.5, 0.5);
	Label* cur_val = manage (new Label ());
	cur_val->set_markup ("<span size=\"x-large\" weight=\"bold\" foreground=\"#FF6B6B\">" + _current_version + "</span>");
	cur_val->set_alignment (0.5, 0.5);
	cur_card->pack_start (*cur_title, false, false, 0);
	cur_card->pack_start (*cur_val, false, false, 4);

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

	_info_label.set_markup ("<span size=\"small\" foreground=\"#AAAAAA\">Se descargarán los nuevos archivos y se aplicarán al reiniciar.</span>");
	_info_label.set_alignment (0.5, 0.5);
	_info_label.set_line_wrap (true);
	body_box->pack_start (_info_label, false, false, 0);

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

#ifdef PLATFORM_WINDOWS
	// === LÓGICA DE ACTUALIZACIÓN EN WINDOWS (WIN32) ===
	string temp_dir = getenv("TEMP") ? getenv("TEMP") : "C:\\Windows\\Temp";
	string zip_file = temp_dir + "\\nova_update.zip";
	string bat_file = temp_dir + "\\nova_updater.bat";

	// Descargar con curl.exe nativo de Windows
	string dl_cmd = "curl.exe -fL -o \"" + zip_file + "\" \"" + _download_url + "\"";
	int dl_ret = system(dl_cmd.c_str());

	if (dl_ret != 0) {
		_progress_bar.set_fraction (0.0);
		_progress_bar.set_text (_("Error en la descarga"));
		_skip_button.set_label (_("Cerrar"));
		_skip_button.set_sensitive (true);
		return;
	}

	_progress_bar.set_fraction (0.8);
	_progress_bar.set_text (_("Generando script de reinicio..."));
	while (gtk_events_pending ()) gtk_main_iteration ();

	// Script script .bat que espera a que NOVA-STUDIO se cierre, descomprime y reinicia
	ofstream bat(bat_file.c_str());
	if (bat) {
		bat << "@echo off\n";
		bat << "timeout /t 2 /nobreak > NUL\n";
		bat << "tar.exe -xf \"" << zip_file << "\" -C \"%~dp0..\"\n";
		bat << "del /f /q \"" << zip_file << "\"\n";
		bat << "start \"\" \"%~dp0ardour9.exe\"\n";
		bat << "del /f /q \"%~f0\"\n";
		bat.close();
	}

	_progress_bar.set_fraction (1.0);
	_progress_bar.set_text (_("¡Descargado! Reiniciando NOVA-STUDIO..."));
	while (gtk_events_pending ()) gtk_main_iteration ();

	// Ejecutar script .bat en segundo plano y cerrar la aplicación actual
	WinExec(("cmd.exe /c start /b " + bat_file).c_str(), SW_HIDE);
	exit(0);

#else
	// === LÓGICA DE ACTUALIZACIÓN EN LINUX / WSL2 ===
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
#endif
}

void
UpdateChecker::check_and_notify (const string& current_version, const string& github_repo)
{
	if (_already_checked) {
		return;
	}
	_already_checked = true;

	std::thread ([current_version, github_repo]() {
		std::this_thread::sleep_for(std::chrono::seconds(1));

		string cmd = "curl -s --max-time 4 \"https://api.github.com/repos/" + github_repo + "/releases/latest\" 2>/dev/null";
		FILE* pipe = popen (cmd.c_str (), "r");
		string response_json = "";
		if (pipe) {
			char buffer[2048];
			while (fgets (buffer, sizeof (buffer), pipe) != NULL) response_json += buffer;
			pclose (pipe);
		}

		string remote_ver = "";
		string dl_url = "";

		size_t tag_pos = response_json.find ("\"tag_name\"");
		if (tag_pos != string::npos) {
			size_t fq = response_json.find ("\"", tag_pos + 10);
			size_t sq = response_json.find ("\"", fq + 1);
			if (fq != string::npos && sq != string::npos) {
				remote_ver = response_json.substr (fq + 1, sq - fq - 1);
			}
		}

		if (strip_v(remote_ver) == strip_v(current_version) || remote_ver.empty()) {
			return; // Ya está actualizado
		}

		// Buscar extensión según la plataforma (.zip para Windows, .tar.gz para Linux)
#ifdef PLATFORM_WINDOWS
		string ext = ".zip";
#else
		string ext = ".tar.gz";
#endif
		size_t url_pos = response_json.find (ext);
		if (url_pos != string::npos) {
			size_t url_start = response_json.rfind ("\"browser_download_url\"", url_pos);
			if (url_start != string::npos) {
				size_t fq = response_json.find ("\"", url_start + 22);
				size_t sq = response_json.find ("\"", fq + 1);
				if (fq != string::npos && sq != string::npos) {
					dl_url = response_json.substr (fq + 1, sq - fq - 1);
				}
			}
		}

		Glib::signal_timeout ().connect (
			sigc::bind (sigc::ptr_fun (&UpdateChecker::idle_show_dialog),
						current_version, remote_ver, dl_url),
			500);
	}).detach ();
}

bool
UpdateChecker::idle_show_dialog (string current, string remote, string url)
{
	UpdateChecker* dialog = new UpdateChecker (current, remote, url);
	dialog->show ();
	dialog->present ();
	dialog->run ();
	delete dialog;
	return false;
}
