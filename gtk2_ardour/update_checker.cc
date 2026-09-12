#include "update_checker.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <glibmm/main.h>
#include <ytk/ytk.h> // Header nativo del motor de UI de Ardour (YTK)

using namespace Gtk;
using namespace std;

UpdateChecker::UpdateChecker(const string& current_version, 
                             const string& remote_version, 
                             const string& download_url)
    : ArdourDialog("NOVA-STUDIO - Actualización Disponible", true, false)
    , _current_version(current_version)
    , _remote_version(remote_version)
    , _download_url(download_url)
    , _update_button("Actualizar ahora")
    , _skip_button("Omitir")
{
    // Tamaño de la ventana del diálogo
    set_default_size(380, 180);
    set_resizable(false);

    // Obtener contenedor vertical principal de ArdourDialog
    VBox* vbox = get_vbox();
    vbox->set_spacing(12);

    // Etiqueta de título
    _message_label.set_markup("<span size=\"large\" weight=\"bold\">¡Nueva actualización disponible!</span>");
    _message_label.set_padding(10, 10);
    vbox->pack_start(_message_label, false, false, 0);

    // Detalles de versiones
    _version_label.set_markup("Versión instalada: <span color=\"#FF5555\">" + _current_version + "</span>\n"
                              "Nueva versión: <span color=\"#55FF55\">" + _remote_version + "</span>");
    _version_label.set_padding(5, 5);
    vbox->pack_start(_version_label, false, false, 0);

    // Barra de progreso (inicialmente oculta)
    _progress_bar.set_text("Esperando descarga...");
    _progress_bar.set_fraction(0.0);
    _progress_bar.set_visible(false);
    vbox->pack_start(_progress_bar, false, false, 5);

    // Caja de botones horizontales
    HBox* button_box = Gtk::manage(new HBox(true, 15));
    button_box->pack_start(_update_button);
    button_box->pack_start(_skip_button);
    vbox->pack_start(*button_box, false, false, 10);

    // Señales de botones
    _update_button.signal_clicked().connect(sigc::mem_fun(*this, &UpdateChecker::on_update_clicked));
    _skip_button.signal_clicked().connect(sigc::mem_fun(*this, &UpdateChecker::on_skip_clicked));

    show_all_children();
}

UpdateChecker::~UpdateChecker() {}

void UpdateChecker::on_update_clicked()
{
    _update_button.set_sensitive(false);
    _skip_button.set_sensitive(false);
    _progress_bar.set_visible(true);
    _progress_bar.set_fraction(0.1);
    _progress_bar.set_text("Iniciando descarga...");

    // Refrescar ventana gráfica de GTK/YTK
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }

    download_and_install();
}

void UpdateChecker::on_skip_clicked()
{
    response(RESPONSE_CANCEL);
}

void UpdateChecker::download_and_install()
{
    _progress_bar.set_fraction(0.3);
    _progress_bar.set_text("Descargando desde GitHub...");
    
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }

    string tmp_dir = "/tmp/nova_upgrade_" + to_string(getpid());
    string dl_cmd = "mkdir -p " + tmp_dir + " && curl -L -o " + tmp_dir + "/update.tar.gz \"" + _download_url + "\" 2>/dev/null";
    
    int dl_ret = system(dl_cmd.c_str());
    if (dl_ret != 0) {
        _progress_bar.set_text("Error en descarga.");
        _skip_button.set_sensitive(true);
        _skip_button.set_label("Cerrar");
        return;
    }

    _progress_bar.set_fraction(0.7);
    _progress_bar.set_text("Instalando archivos (sudo)...");
    
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }

    // Extraer en /usr/local
    string install_cmd = "sudo tar -xzf " + tmp_dir + "/update.tar.gz -C /usr/local/ 2>/dev/null";
    int inst_ret = system(install_cmd.c_str());

    _progress_bar.set_fraction(1.0);
    if (inst_ret == 0) {
        _progress_bar.set_text("¡Listo! Reinicia para aplicar.");
        
        // Guardar la nueva versión localmente
        string version_file = string(getenv("HOME")) + "/.nova_studio_version";
        ofstream vf(version_file);
        if (vf) vf << _remote_version;
    } else {
        _progress_bar.set_text("Fallo al escribir en /usr/local");
    }

    // Limpieza
    system(("rm -rf " + tmp_dir).c_str());

    _skip_button.set_sensitive(true);
    _skip_button.set_label("Cerrar");
}

// === Lógica en segundo plano (Asíncrona) ===

void UpdateChecker::check_and_notify(const string& current_version, const string& github_repo)
{
    // Ejecutar hilo de fondo para no bloquear el inicio de Ardour
    std::thread(thread_worker, current_version, github_repo).detach();
}

void UpdateChecker::thread_worker(string current_version, string github_repo)
{
    // Esperar a que la UI de Ardour esté completamente cargada
    sleep(2);

    string cmd = "curl -s --max-time 4 \"https://api.github.com/repos/" + github_repo + "/releases/latest\" 2>/dev/null";
    
    // Leer respuesta API
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return;

    char buffer[1024];
    string response_json = "";
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        response_json += buffer;
    }
    pclose(pipe);

    if (response_json.empty()) return;

    // Extraer "tag_name"
    size_t tag_pos = response_json.find("\"tag_name\"");
    if (tag_pos == string::npos) return;
    
    size_t first_quote = response_json.find("\"", tag_pos + 10);
    size_t second_quote = response_json.find("\"", first_quote + 1);
    string remote_version = response_json.substr(first_quote + 1, second_quote - first_quote - 1);

    // Comparar versiones limpiando la 'v' si la tienen
    string v_rem = remote_version;
    string v_loc = current_version;
    if (v_rem[0] == 'v') v_rem = v_rem.substr(1);
    if (v_loc[0] == 'v') v_loc = v_loc.substr(1);

    if (v_rem == v_loc) {
        return; // Ya está actualizado
    }

    // Extraer "browser_download_url" del asset .tar.gz
    size_t url_pos = response_json.find(".tar.gz");
    if (url_pos == string::npos) return;

    size_t url_start = response_json.rfind("\"browser_download_url\"", url_pos);
    size_t url_val_start = response_json.find("\"", url_start + 22);
    size_t url_val_end = response_json.find("\"", url_val_start + 1);
    string download_url = response_json.substr(url_val_start + 1, url_val_end - url_val_start - 1);

    // Enviar señal al hilo principal de GTK de forma segura
    Glib::signal_idle().connect(sigc::bind(sigc::ptr_fun(&UpdateChecker::idle_show_dialog), current_version, remote_version, download_url));
}

bool UpdateChecker::idle_show_dialog(string current, string remote, string url)
{
    UpdateChecker* dialog = new UpdateChecker(current, remote, url);
    dialog->run();
    delete dialog;
    return false; // Desconectar handler de idle automáticamente
}
