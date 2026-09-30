#include "NovaSessionManager.h"
#include "NovaLogging.h"
#include "NovaTimeUtils.h"
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QSettings>
#include <QDateTime>

#pragma push_macro("emit")
#pragma push_macro("slots")
#pragma push_macro("signals")
#pragma push_macro("foreach")
#undef emit
#undef slots
#undef signals
#undef foreach

#include "ardour/ardour.h"
#include "ardour/audioengine.h"
#include "ardour/audio_backend.h"
#include "ardour/session.h"
#include "ardour/session_configuration.h"
#include "ardour/session_event.h"
#include "ardour/plugin_manager.h"
#include "ardour/types.h"

#pragma pop_macro("emit")
#pragma pop_macro("slots")
#pragma pop_macro("signals")
#pragma pop_macro("foreach")

NovaSessionManager::NovaSessionManager(QObject *parent)
    : QObject(parent)
{
}

NovaSessionManager::~NovaSessionManager()
{
    closeCurrentSession();
}

bool NovaSessionManager::initSession()
{
    if (m_session) return true;

    qCDebug(novaCore) << "Inicializando subsistemas PBD y Ardour Core...";

    try {
        if (!ARDOUR::init(true, nullptr, true)) {
            qCCritical(novaCore) << "Error al inicializar subsistemas globales ARDOUR::init";
            return false;
        }

        if (!ARDOUR::SessionEvent::has_per_thread_pool()) {
            ARDOUR::SessionEvent::create_per_thread_pool("GUI", 4096);
        }

        ARDOUR::PluginManager::instance().refresh(true);

        if (!m_engine) {
            m_engine = ARDOUR::AudioEngine::create();
        }

        if (!m_engine) {
            qCCritical(novaCore) << "Error fatal: No se pudo instanciar ARDOUR::AudioEngine.";
            return false;
        }

        m_engine->set_backend("JACK/Pipewire", "NOVA STUDIO", "");
        qCDebug(novaCore) << "Backend JACK/Pipewire instanciado. Configurando parámetros...";

        if (m_engine->current_backend()) {
            m_engine->current_backend()->set_sample_rate(48000.0f);
            m_engine->current_backend()->set_buffer_size(1024);
        }

        if (m_engine->start() != 0) {
            qCCritical(novaCore) << "Error al arrancar el motor de audio.";
            return false;
        }

        qCDebug(novaCore) << "🔊 Conectado exitosamente al servidor PipeWire-JACK!";

        // Generar sesión limpia por defecto
        return createNewSession("DefaultSession", QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/nova_ui_qt");

    } catch (const std::exception &e) {
        qCCritical(novaCore) << "Excepción al inicializar el gestor de sesiones:" << e.what();
    }

    return false;
}

// 💾 GUARDA EL PROYECTO ACTUAL
bool NovaSessionManager::saveSession()
{
    if (!m_session) return false;

    try {
        qCDebug(novaCore) << "💾 Guardando estado de la sesión de Ardour...";
        int result = m_session->save_state(""); // Guarda la snapshot actual
        if (result == 0) {
            qCDebug(novaCore) << "💾 [OK] Proyecto guardado exitosamente en:" << QString::fromStdString(m_session->path());
            
            // Registrar en historial de proyectos recientes
            addProjectToRecent(QString::fromStdString(m_session->name()), QString::fromStdString(m_session->path()));
            return true;
        } else {
            qCCritical(novaCore) << "❌ Error al guardar sesión. Código de retorno Ardour:" << result;
        }
    } catch (const std::exception &e) {
        qCCritical(novaCore) << "Excepción crítica durante el guardado:" << e.what();
    }
    return false;
}

// ⚠️ DETECTA CAMBIOS SIN GUARDAR (DIRTY)
bool NovaSessionManager::isDirty() const
{
    if (!m_session) return false;
    return m_session->dirty();
}

// 📂 CIERRA LA SESIÓN ACTIVA (NATIVO ARDOUR)
void NovaSessionManager::closeCurrentSession()
{
    if (m_session) {
        qCDebug(novaCore) << "🚪 Cerrando sesión activa de Ardour...";
        // Ardour utiliza señales DropReferences durante delete m_session para limpiar m_engine automáticamente
        delete m_session;
        m_session = nullptr;
    }
}

// 📂 ABRE UN PROYECTO EXISTENTE DESDE DISCO
bool NovaSessionManager::loadSession(const QString &sessionPath)
{
    if (!m_engine) return false;

    qCDebug(novaCore) << "📂 Intentando cargar proyecto existente en:" << sessionPath;

    try {
        closeCurrentSession();

        QFileInfo checkFile(sessionPath);
        QString realSessionPath = sessionPath;
        
        if (checkFile.isFile() && checkFile.suffix() == "ardour") {
            realSessionPath = checkFile.absolutePath();
        }

        std::string pathStd = realSessionPath.toStdString();
        std::string nameStd = QDir(realSessionPath).dirName().toStdString();

        ARDOUR::BusProfile bus_profile;
        bus_profile.master_out_channels = 2;

        m_session = new ARDOUR::Session(*m_engine, pathStd, nameStd, &bus_profile, "", false);

        if (m_session) {
            m_engine->set_session(m_session);
            applySessionSafetyConfig();
            
            qCDebug(novaCore) << "📂 [OK] Proyecto cargado con éxito:" << realSessionPath;
            addProjectToRecent(QString::fromStdString(nameStd), realSessionPath);
            
            Q_EMIT sessionInitialized(m_session);
            return true;
        }
    } catch (const std::exception &e) {
        qCCritical(novaCore) << "❌ Error al abrir la sesión:" << e.what();
    }
    return false;
}

// 📂 CREA UN PROYECTO TOTALMENTE NUEVO
bool NovaSessionManager::createNewSession(const QString &projectName, const QString &parentDir)
{
    if (!m_engine || projectName.isEmpty() || parentDir.isEmpty()) return false;

    QString sessionDir = parentDir + "/" + projectName;
    qCDebug(novaCore) << "🆕 Generando proyecto nuevo en:" << sessionDir;

    try {
        closeCurrentSession();

        if (QDir(sessionDir).exists()) {
            QDir(sessionDir).removeRecursively();
        }
        QDir().mkpath(parentDir);

        ARDOUR::BusProfile bus_profile;
        bus_profile.master_out_channels = 2;

        m_session = new ARDOUR::Session(*m_engine, sessionDir.toStdString(), projectName.toStdString(), &bus_profile, "", true);

        if (m_session) {
            m_engine->set_session(m_session);
            applySessionSafetyConfig();

            qCDebug(novaCore) << "🆕 [OK] Nuevo proyecto de Ardour instanciado correctamente.";
            addProjectToRecent(projectName, sessionDir);

            Q_EMIT sessionInitialized(m_session);
            return true;
        }
    } catch (const std::exception &e) {
        qCCritical(novaCore) << "❌ Excepción durante la creación del proyecto:" << e.what();
    }
    return false;
}

void NovaSessionManager::applySessionSafetyConfig()
{
    if (m_session) {
        m_session->config.set_session_monitoring(ARDOUR::MonitorDisk);
    }
}

// 📊 OBTENER LISTA DE PROYECTOS RECIENTES
QVariantList NovaSessionManager::getRecentProjects() const
{
    QSettings settings("NovaStudio", "DAW");
    return settings.value("recentProjects").toList();
}

// 📊 AGREGAR PROYECTO AL HISTORIAL DE RECIENTES
void NovaSessionManager::addProjectToRecent(const QString &name, const QString &path)
{
    QSettings settings("NovaStudio", "DAW");
    QVariantList list = settings.value("recentProjects").toList();

    for (int i = 0; i < list.size(); ++i) {
        QVariantMap item = list[i].toMap();
        if (item["path"].toString() == path) {
            list.removeAt(i);
            break;
        }
    }

    QVariantMap projectMap;
    projectMap["name"] = name;
    projectMap["path"] = path;
    projectMap["lastModified"] = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm AP");

    list.prepend(projectMap);

    while (list.size() > 10) {
        list.removeLast();
    }

    settings.setValue("recentProjects", list);
    qCDebug(novaCore) << "📊 Historial de Proyectos Recientes Actualizado:" << name << "->" << path;
    
    Q_EMIT recentProjectsChanged();
}