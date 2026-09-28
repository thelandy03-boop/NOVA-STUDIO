#include "NovaSessionManager.h"
#include "NovaLogging.h"
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QCoreApplication>
#include <vector>

#pragma push_macro("emit")
#pragma push_macro("slots")
#pragma push_macro("signals")
#pragma push_macro("foreach")
#undef emit
#undef slots
#undef signals
#undef foreach

#include "pbd/pbd.h"
#include "ardour/ardour.h"
#include "ardour/audioengine.h"
#include "ardour/session.h"
#include "ardour/session_event.h"
#include "ardour/filesystem_paths.h"
#include "ardour/audio_backend.h"
#include "ardour/rc_configuration.h"

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
        m_session->request_stop();
        if (m_engine) {
            m_engine->remove_session();
        }
        delete m_session;
        m_session = nullptr;
    }

    if (m_engine) {
        m_engine->stop();
        m_engine = nullptr;
    }
}

bool NovaSessionManager::initSession()
{
    qCDebug(novaCore) << "Inicializando subsistemas PBD y Ardour Core...";

    QString rootDir = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../..");
    if (qEnvironmentVariableIsEmpty("ARDOUR_CONFIG_PATH")) {
        qputenv("ARDOUR_CONFIG_PATH", QString("%1/system_config:%1/share").arg(rootDir).toUtf8());
    }

    if (!PBD::init()) {
        qCCritical(novaCore) << "Fallo al inicializar PBD.";
        return false;
    }

    ARDOUR::init(false, "", false);

    try {
        ARDOUR::SessionEvent::create_per_thread_pool("NovaUI", 1024);
        qCDebug(novaCore) << "Piscina lock-free inicializada.";
    } catch (const std::exception &e) {
        qCCritical(novaCore) << "Error en piscina de eventos:" << e.what();
        return false;
    }

    if (ARDOUR::Config) {
        ARDOUR::Config->set_stop_at_session_end(false);
    }

    m_engine = ARDOUR::AudioEngine::create();
    if (!m_engine) {
        qCCritical(novaCore) << "Imposible instanciar ARDOUR::AudioEngine.";
        return false;
    }

    // 🔊 CONFIGURACIÓN DE BACKEND CON ARRANQUE GARANTIZADO
    bool engineStarted = false;

    // 1. Probar JACK / Pipewire
    std::shared_ptr<ARDOUR::AudioBackend> jackBackend = m_engine->set_backend("JACK/Pipewire", "NOVA-Studio", "");
    if (!jackBackend) {
        jackBackend = m_engine->set_backend("JACK", "NOVA-Studio", "");
    }

    if (jackBackend) {
        qCDebug(novaCore) << "Backend JACK/Pipewire instanciado. Configurando parámetros...";
        jackBackend->set_sample_rate(48000.0f);
        jackBackend->set_buffer_size(1024);

        if (m_engine->start() == 0) {
            qCDebug(novaCore) << "🔊 Conectado exitosamente al servidor PipeWire-JACK!";
            engineStarted = true;
        } else {
            qCWarning(novaCore) << "m_engine->start() falló con backend JACK. Probando alternativas...";
        }
    }

    // 2. Si JACK no arranca, probar ALSA
    if (!engineStarted) {
        qCWarning(novaCore) << "Probando backend ALSA...";
        std::shared_ptr<ARDOUR::AudioBackend> alsaBackend = m_engine->set_backend("ALSA", "NOVA-Studio", "");
        if (alsaBackend) {
            alsaBackend->set_sample_rate(48000.0f);
            alsaBackend->set_buffer_size(1024);
            alsaBackend->set_device_name("default");
            if (m_engine->start() == 0) {
                engineStarted = true;
            }
        }
    }

    // 3. Fallback de seguridad (Dummy) para asegurar que el engine SIEMPRE esté running()
    if (!engineStarted) {
        qCWarning(novaCore) << "⚠️ Cayendo a backend de seguridad 'None (Dummy)' para garantizar ejecución...";
        std::shared_ptr<ARDOUR::AudioBackend> dummy = m_engine->set_backend("None (Dummy)", "NOVA-Studio", "");
        if (dummy) {
            dummy->set_sample_rate(48000.0f);
            dummy->set_buffer_size(1024);
            if (m_engine->start() == 0) {
                engineStarted = true;
            }
        }
    }

    if (!engineStarted || !m_engine->running()) {
        qCCritical(novaCore) << "❌ Error fatal: El motor de audio no está corriendo.";
        return false;
    }

    qCDebug(novaCore) << "Engine activo y corriendo (" << QString::fromStdString(m_engine->current_backend_name()) << "). Construyendo ARDOUR::Session...";

    QString basePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(basePath);
    QString sessionDir = basePath + "/DefaultSession";
    QString stateFile = sessionDir + "/DefaultSession.ardour";

    // 🧹 LIMPIEZA ABSOLUTA: Borrar siempre la sesión Default anterior para iniciar limpio
    if (QDir(sessionDir).exists()) {
        qCDebug(novaCore) << "Limpiando sesión Default anterior para garantizar arranque limpio...";
        QDir(sessionDir).removeRecursively();
    }

    try {
        // 🎛️ Configurar un Bus Profile estéreo para activar el Master Bus nativo de Ardour
        ARDOUR::BusProfile bus_profile;
        bus_profile.master_out_channels = 2; 

        m_session = new ARDOUR::Session(*m_engine, sessionDir.toStdString(), "DefaultSession", &bus_profile, "", true);
        m_engine->set_session(m_session);

        m_session->set_session_range_is_free(true);
        m_session->set_session_extents(Temporal::timepos_t(0), Temporal::timepos_t(Temporal::max_samplepos));

        m_session->request_locate(0);

        qCDebug(novaCore) << "🔊 ARDOUR::Session activa y lista en:" << sessionDir;
        Q_EMIT sessionInitialized(m_session);
        return true;
    } catch (const std::exception &e) {
        qCCritical(novaCore) << "Excepción al crear la sesión:" << e.what();
        return false;
    } catch (...) {
        qCCritical(novaCore) << "Error desconocido al crear la sesión.";
        return false;
    }
}