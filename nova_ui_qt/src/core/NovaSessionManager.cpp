#include "NovaSessionManager.h"
#include "NovaLogging.h"
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QCoreApplication>

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

    std::shared_ptr<ARDOUR::AudioBackend> backend = m_engine->set_backend("None (Dummy)", "NOVA-Studio", "");
    if (!backend) {
        qCCritical(novaCore) << "Imposible cargar backend Dummy (None (Dummy)).";
        return false;
    }

    backend->set_driver("Realtime");
    backend->set_sample_rate(44100.0f);
    backend->set_buffer_size(512);
    qCDebug(novaCore) << "Backend None (Dummy) 44.1kHz / 512 samples activo.";

    if (m_engine->start() != 0) {
        qCCritical(novaCore) << "Fallo al arrancar ARDOUR::AudioEngine.";
        return false;
    }
    qCDebug(novaCore) << "Engine arrancado:" << QString::fromStdString(m_engine->current_backend_name());

    QString basePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(basePath);
    QString sessionDir = basePath + "/DefaultSession";
    QString stateFile = sessionDir + "/DefaultSession.ardour";

    // 🔒 Limpieza de sesión incompleta/corrupta
    if (QDir(sessionDir).exists() && !QFileInfo::exists(stateFile)) {
        qCWarning(novaCore) << "Sesión incompleta detectada, limpiando:" << sessionDir;
        QDir(sessionDir).removeRecursively();
    }

    try {
        m_session = new ARDOUR::Session(*m_engine, sessionDir.toStdString(), "DefaultSession", 0, "", true);
        m_engine->set_session(m_session);

        m_session->set_session_range_is_free(true);
        m_session->set_session_extents(Temporal::timepos_t(0), Temporal::timepos_t(Temporal::max_samplepos));

        // Asegurar playhead en 0 al arrancar
        m_session->request_locate(0);

        qCDebug(novaCore) << "ARDOUR::Session activa en:" << sessionDir;
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