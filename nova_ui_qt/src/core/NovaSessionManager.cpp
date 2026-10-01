#include "NovaSessionManager.h"
#include "NovaLogging.h"
#include "NovaProjectManager.h"
#include "platform/NovaPlatformUtils.h"
#include "platform/NovaAndroidStubs.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <thread>
#include <chrono>

#if !defined(Q_OS_ANDROID)
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
#include "ardour/session_event.h"
#include "ardour/plugin_manager.h"

#pragma pop_macro("emit")
#pragma pop_macro("slots")
#pragma pop_macro("signals")
#pragma pop_macro("foreach")
#endif

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
    if (m_engine) return true;

    qCDebug(novaCore) << "Inicializando subsistemas PBD y Ardour Core...";

    try {
#if !defined(Q_OS_ANDROID)
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

        int retries = 0;
        while (!m_engine->current_backend() && retries < 5) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            m_engine->set_backend("JACK/Pipewire", "NOVA STUDIO", "");
            retries++;
        }

        auto backend = m_engine->current_backend();
        if (!backend) {
            qCCritical(novaCore) << "❌ Error fatal: El backend de audio JACK/Pipewire no respondió.";
            return false;
        }

        backend->set_sample_rate(48000.0f);
        backend->set_buffer_size(1024);

        if (m_engine->start() != 0) {
            qCCritical(novaCore) << "Error al arrancar el motor de audio.";
            return false;
        }

        qCDebug(novaCore) << "🔊 Conectado exitosamente al servidor PipeWire-JACK!";
#else
        qCDebug(novaCore) << "📱 Inicializando subsistema C++ para Android...";
        if (!m_engine) {
            m_engine = ARDOUR::AudioEngine::create();
        }
#endif
        return true;

    } catch (const std::exception &e) {
        qCCritical(novaCore) << "Excepción al inicializar el gestor de sesiones:" << e.what();
    }

    return false;
}

void NovaSessionManager::closeCurrentSession()
{
    if (m_session) {
        qCDebug(novaCore) << "🚪 Cerrando sesión activa de Ardour...";
        delete m_session;
        m_session = nullptr;
    }
}