#include "NovaAudioEngine.h"
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QCoreApplication>
#include <cmath>

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

struct _VSTState;
int vstfx_init(void*) { return 0; }
void vstfx_exit() {}
void vstfx_destroy_editor(_VSTState*) {}

NovaAudioEngine::NovaAudioEngine(QObject *parent)
    : QObject(parent)
{
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(16); // ~60 FPS
    connect(m_updateTimer, &QTimer::timeout, this, &NovaAudioEngine::updatePositionFromArdour);
}

NovaAudioEngine::~NovaAudioEngine()
{
    if (m_updateTimer) {
        m_updateTimer->stop();
    }

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

bool NovaAudioEngine::initEngine()
{
    qDebug() << "⚡ [NOVA ENGINE] Inicializando subsistemas PBD y Ardour Core...";

    // 1. Configurar rutas de entorno obligatorias
    QString rootDir = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../..");
    if (qEnvironmentVariableIsEmpty("ARDOUR_CONFIG_PATH")) {
        qputenv("ARDOUR_CONFIG_PATH", QString("%1/system_config:%1/share").arg(rootDir).toUtf8());
    }

    // 2. Inicializar base PBD y Ardour
    if (!PBD::init()) {
        qCritical() << "❌ [NOVA ENGINE] Fallo al inicializar PBD";
        return false;
    }

    ARDOUR::init(false, "", false);

    // 3. Registrar el hilo principal de Qt en el pool de asignación lock-free de Ardour
    try {
        ARDOUR::SessionEvent::create_per_thread_pool("NovaUI", 1024);
        qDebug() << "✅ [NOVA ENGINE] Registrada piscina de eventos de memoria para el hilo principal (NovaUI)";
    } catch (std::exception &e) {
        qCritical() << "❌ [NOVA ENGINE] Fallo crítico al registrar la piscina de eventos de memoria:" << e.what();
        return false;
    }

    // 4. Desactivar la parada automática global en fin de sesión
    if (ARDOUR::Config) {
        ARDOUR::Config->set_stop_at_session_end(false);
    }

    // 5. Instanciar AudioEngine
    m_engine = ARDOUR::AudioEngine::create();
    if (!m_engine) {
        qCritical() << "❌ [NOVA ENGINE] No se pudo instanciar ARDOUR::AudioEngine";
        return false;
    }

    // 6. Cargar y configurar Backend de Audio (Dummy)
    std::shared_ptr<ARDOUR::AudioBackend> backend = m_engine->set_backend("None (Dummy)", "NOVA-Studio", "");
    if (!backend) {
        qCritical() << "❌ [NOVA ENGINE] No se pudo cargar el backend de audio Dummy";
        return false;
    }

    backend->set_driver("Realtime");
    backend->set_sample_rate(44100.0f);
    backend->set_buffer_size(512);
    qDebug() << "✅ [NOVA ENGINE] Backend configurado en tiempo real (44.1kHz / 512 samples)";

    // 7. ORDEN OBLIGATORIO: Iniciar el motor en tiempo real ANTES de crear la Sesión
    if (m_engine->start() != 0) {
        qCritical() << "❌ [NOVA ENGINE] Fallo al iniciar el procesamiento de ARDOUR::AudioEngine";
        return false;
    }
    qDebug() << "✅ [NOVA ENGINE] ARDOUR::AudioEngine arrancado en backend:" << QString::fromStdString(m_engine->current_backend_name());

    // 8. Determinar la ruta de la sesión de manera limpia
    QString basePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(basePath);
    QString sessionDir = basePath + "/DefaultSession";
    QString stateFile = sessionDir + "/DefaultSession.ardour";

    if (QDir(sessionDir).exists() && !QFileInfo::exists(stateFile)) {
        QDir(sessionDir).removeRecursively();
    }

    // 9. Instanciar ARDOUR::Session y vincularla al motor ya en ejecución
    try {
        m_session = new ARDOUR::Session(*m_engine, sessionDir.toStdString(), "DefaultSession", 0, "", true);
        m_engine->set_session(m_session);

        // 10. CONFIGURACIÓN DE EXTENSIÓN INFINITA: Fijar rango final a max_samplepos
        m_session->set_session_range_is_free(true);
        m_session->set_session_extents(Temporal::timepos_t(0), Temporal::timepos_t(Temporal::max_samplepos));

        qDebug() << "✅ [NOVA ENGINE] ARDOUR::Session creada y vinculada con éxito en:" << sessionDir;
        qDebug() << "✅ [NOVA ENGINE] Extensión del Timeline fijada a máxima duración (Reproducción infinita activa)";
    } catch (std::exception &e) {
        qCritical() << "❌ [NOVA ENGINE] Excepción al instanciar ARDOUR::Session:" << e.what();
        return false;
    } catch (...) {
        qCritical() << "❌ [NOVA ENGINE] Error desconocido al instanciar ARDOUR::Session";
        return false;
    }

    m_updateTimer->start();
    return true;
}

void NovaAudioEngine::play()
{
    if (m_session) {
        m_session->request_roll();
    }
    m_isPlaying = true;
    Q_EMIT isPlayingChanged();
    qDebug() << "▶️ [ARDOUR / ENGINE] Transport: ROLL (PLAY)";
}

void NovaAudioEngine::stop()
{
    if (m_session) {
        if (m_session->transport_speed() != 0.0 || m_session->transport_rolling()) {
            m_session->request_stop();
        } else {
            m_session->request_locate(0);
        }
    }
    
    if (m_isPlaying) {
        m_isPlaying = false;
        Q_EMIT isPlayingChanged();
        qDebug() << "⏹️ [ARDOUR / ENGINE] Transport: STOP";
    }
}

void NovaAudioEngine::rewind()
{
    if (m_session) {
        m_session->request_locate(0);
    }
    m_currentFrame = 0.0;
    m_timecode = "00:00:00";
    m_bbt = "001 : 01 : 01";
    Q_EMIT timecodeChanged();
    Q_EMIT bbtChanged();
    Q_EMIT positionChanged();
    qDebug() << "⏮️ [ARDOUR / ENGINE] Transport: REWIND -> Compás 1";
}

void NovaAudioEngine::togglePlay()
{
    if (m_isPlaying) stop();
    else play();
}

void NovaAudioEngine::toggleRecord()
{
    if (m_session) {
        if (m_session->get_record_enabled()) {
            m_session->disable_record(false, false);
            m_isRecording = false;
        } else {
            m_session->maybe_enable_record();
            m_isRecording = true;
        }
    } else {
        m_isRecording = !m_isRecording;
    }
    Q_EMIT isRecordingChanged();
    qDebug() << "🔴 [ARDOUR / ENGINE] Transport: RECORD ->" << m_isRecording;
}

void NovaAudioEngine::setBpm(double newBpm)
{
    if (qFuzzyCompare(m_bpm, newBpm)) return;
    m_bpm = newBpm;
    Q_EMIT bpmChanged();
}

void NovaAudioEngine::updatePositionFromArdour()
{
    if (!m_session) return;

    // Leer posición real en samples del transporte fundamental
    ARDOUR::samplepos_t pos = m_session->transport_sample();
    ARDOUR::samplecnt_t sr = m_session->sample_rate();
    if (sr <= 0) sr = 44100;

    m_currentFrame = static_cast<double>(pos);
    bool rolling = m_session->transport_rolling() || (m_session->transport_speed() != 0.0);
    bool recArm = m_session->get_record_enabled();

    // FORMATO DE LOGS INTELIGENTE CON ACUMULADOR [xN]
    static QString lastLoggedMsg = "";
    static int repeatCount = 1;

    if (rolling) {
        QString currentMsg = QString("⏱️ [MOTOR ROLL] Frame: %1 | BBT: %2").arg(pos).arg(m_bbt);
        if (currentMsg == lastLoggedMsg) {
            repeatCount++;
            if (repeatCount % 60 == 0) {
                qDebug().noquote() << QString("%1 [x%2]").arg(currentMsg).arg(repeatCount);
            }
        } else {
            if (repeatCount > 1 && !lastLoggedMsg.isEmpty()) {
                qDebug().noquote() << QString("%1 [x%2]").arg(lastLoggedMsg).arg(repeatCount);
            }
            qDebug().noquote() << currentMsg;
            lastLoggedMsg = currentMsg;
            repeatCount = 1;
        }
    } else {
        lastLoggedMsg = "";
        repeatCount = 1;
    }

    if (m_isPlaying != rolling) {
        m_isPlaying = rolling;
        Q_EMIT isPlayingChanged();
    }

    if (m_isRecording != recArm) {
        m_isRecording = recArm;
        Q_EMIT isRecordingChanged();
    }

    // 1. TIMECODE (Minutos : Segundos : Centésimas)
    double currentSeconds = m_currentFrame / static_cast<double>(sr);
    int totalSeconds = static_cast<int>(currentSeconds);
    int mins = totalSeconds / 60;
    int secs = totalSeconds % 60;
    int ms = static_cast<int>((m_currentFrame / (static_cast<double>(sr) / 100.0))) % 100;

    m_timecode = QString("%1:%2:%3")
                 .arg(mins, 2, 10, QChar('0'))
                 .arg(secs, 2, 10, QChar('0'))
                 .arg(ms, 2, 10, QChar('0'));

    // 2. BBT MUSICAL
    double bpm = m_bpm > 0 ? m_bpm : 120.0;
    double totalBeats = currentSeconds * (bpm / 60.0);
    
    int bar = 1 + static_cast<int>(totalBeats / 4.0);
    int beat = 1 + static_cast<int>(std::fmod(totalBeats, 4.0));
    int sub = 1 + static_cast<int>(std::fmod(totalBeats * 4.0, 4.0));

    m_bbt = QString("%1 : %2 : %3")
            .arg(bar, 3, 10, QChar('0'))
            .arg(beat, 2, 10, QChar('0'))
            .arg(sub, 2, 10, QChar('0'));

    Q_EMIT timecodeChanged();
    Q_EMIT bbtChanged();
    Q_EMIT positionChanged();
}