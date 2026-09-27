#include "NovaSessionManager.h"
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
    qDebug() << "⚡ [SESSION MANAGER] Inicializando subsistemas PBD y Ardour Core...";

    QString rootDir = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../..");
    if (qEnvironmentVariableIsEmpty("ARDOUR_CONFIG_PATH")) {
        qputenv("ARDOUR_CONFIG_PATH", QString("%1/system_config:%1/share").arg(rootDir).toUtf8());
    }

    if (!PBD::init()) {
        qCritical() << "❌ [SESSION MANAGER] Fallo al inicializar PBD";
        return false;
    }

    ARDOUR::init(false, "", false);

    try {
        ARDOUR::SessionEvent::create_per_thread_pool("NovaUI", 1024);
        qDebug() << "✅ [SESSION MANAGER] Piscina lock-free inicializada";
    } catch (std::exception &e) {
        qCritical() << "❌ [SESSION MANAGER] Error en piscina de eventos:" << e.what();
        return false;
    }

    if (ARDOUR::Config) {
        ARDOUR::Config->set_stop_at_session_end(false);
    }

    m_engine = ARDOUR::AudioEngine::create();
    if (!m_engine) {
        qCritical() << "❌ [SESSION MANAGER] Imposible instanciar ARDOUR::AudioEngine";
        return false;
    }

    std::shared_ptr<ARDOUR::AudioBackend> backend = m_engine->set_backend("None (Dummy)", "NOVA-Studio", "");
    if (!backend) {
        qCritical() << "❌ [SESSION MANAGER] Imposible cargar backend Dummy";
        return false;
    }

    backend->set_driver("Realtime");
    backend->set_sample_rate(44100.0f);
    backend->set_buffer_size(512);
    qDebug() << "✅ [SESSION MANAGER] Backend Dummy 44.1kHz / 512 samples";

    if (m_engine->start() != 0) {
        qCritical() << "❌ [SESSION MANAGER] Fallo al arrancar ARDOUR::AudioEngine";
        return false;
    }
    qDebug() << "✅ [SESSION MANAGER] Engine arrancado:" << QString::fromStdString(m_engine->current_backend_name());

    QString basePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(basePath);
    QString sessionDir = basePath + "/DefaultSession";
    QString stateFile = sessionDir + "/DefaultSession.ardour";

    // 🔒 Limpieza de sesión incompleta/corrupta (igual que antes de modularizar)
    if (QDir(sessionDir).exists() && !QFileInfo::exists(stateFile)) {
        qWarning() << "⚠️ [SESSION MANAGER] Sesión incompleta detectada, limpiando:" << sessionDir;
        QDir(sessionDir).removeRecursively();
    }

    try {
        m_session = new ARDOUR::Session(*m_engine, sessionDir.toStdString(), "DefaultSession", 0, "", true);
        m_engine->set_session(m_session);

        m_session->set_session_range_is_free(true);
        m_session->set_session_extents(Temporal::timepos_t(0), Temporal::timepos_t(Temporal::max_samplepos));

        // Asegurar playhead en 0 al arrancar
        m_session->request_locate(0);

        qDebug() << "✅ [SESSION MANAGER] ARDOUR::Session activa en:" << sessionDir;
        Q_EMIT sessionInitialized(m_session);
        return true;
    } catch (std::exception &e) {
        qCritical() << "❌ [SESSION MANAGER] Excepción al crear la sesión:" << e.what();
        return false;
    } catch (...) {
        qCritical() << "❌ [SESSION MANAGER] Error desconocido al crear la sesión";
        return false;
    }
}