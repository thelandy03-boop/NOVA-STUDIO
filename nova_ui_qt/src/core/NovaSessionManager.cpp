#include "NovaSessionManager.h"
#include "NovaLogging.h"
#include "NovaTimeUtils.h"
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

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
    if (m_session) {
        delete m_session;
        m_session = nullptr;
    }
}

bool NovaSessionManager::initSession()
{
    if (m_session) return true;

    qCDebug(novaCore) << "Inicializando subsistemas PBD y Ardour Core...";

    try {
        // 🚀 1. Inicialización oficial de subsistemas globales de Ardour
        if (!ARDOUR::init(true, nullptr, true)) {
            qCCritical(novaCore) << "Error al inicializar subsistemas globales ARDOUR::init";
            return false;
        }

        // 🚀 2. Registrar el pool de eventos en el hilo de la interfaz Qt6
        if (!ARDOUR::SessionEvent::has_per_thread_pool()) {
            ARDOUR::SessionEvent::create_per_thread_pool("GUI", 4096);
        }

        // 🚀 3. Poblar la caché de plugins (LV2, LADSPA, VST, Lua)
        ARDOUR::PluginManager::instance().refresh(true);

        // 🚀 4. Instanciar y configurar AudioEngine
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

        // 🚀 5. Preparar ruta de sesión (Ardour requiere que la carpeta NO exista para _is_new = true)
        QString parentDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/nova_ui_qt";
        QString sessionDir = parentDir + "/DefaultSession";

        QDir().mkpath(parentDir);

        if (QDir(sessionDir).exists()) {
            QDir(sessionDir).removeRecursively();
        }

        qCDebug(novaCore) << "Engine activo. Construyendo ARDOUR::Session en:" << sessionDir;

        ARDOUR::BusProfile bus_profile;
        bus_profile.master_out_channels = 2;

        m_session = new ARDOUR::Session(*m_engine, sessionDir.toStdString(), "DefaultSession", &bus_profile, "", true);

        if (m_session) {
            m_engine->set_session(m_session);

            // 🛡️ Desactivar software monitoring por defecto para prevenir acoples
            m_session->config.set_session_monitoring(ARDOUR::MonitorDisk);

            qCDebug(novaCore) << "🔊 ARDOUR::Session activa y lista en:" << sessionDir;
            Q_EMIT sessionInitialized(m_session);
            return true;
        }

    } catch (const std::exception &e) {
        qCCritical(novaCore) << "Excepción al inicializar la sesión de Ardour:" << e.what();
    }

    return false;
}