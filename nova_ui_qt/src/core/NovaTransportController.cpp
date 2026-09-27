#include "NovaTransportController.h"
#include <QDebug>
#include <cmath>
#include <algorithm>

#pragma push_macro("emit")
#pragma push_macro("slots")
#pragma push_macro("signals")
#pragma push_macro("foreach")
#undef emit
#undef slots
#undef signals
#undef foreach

#include "ardour/session.h"
#include "ardour/location.h"

#pragma pop_macro("emit")
#pragma pop_macro("slots")
#pragma pop_macro("signals")
#pragma pop_macro("foreach")

NovaTransportController::NovaTransportController(QObject *parent)
    : QObject(parent)
{
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(16); // ~60 FPS
    connect(m_updateTimer, &QTimer::timeout, this, &NovaTransportController::updatePositionFromArdour);
}

void NovaTransportController::setSession(ARDOUR::Session *session)
{
    m_session = session;
    if (m_session) {
        m_updateTimer->start();
        qDebug() << "✅ [TRANSPORT] Sesión vinculada, timer 60 FPS activo";
    } else {
        m_updateTimer->stop();
    }
}

void NovaTransportController::play()
{
    if (!m_session) return;

    m_locatePending = false;
    m_session->request_roll();
    m_isPlaying = true;
    Q_EMIT isPlayingChanged();
    qDebug() << "▶️ [TRANSPORT] ROLL solicitado";
}

void NovaTransportController::stop()
{
    bool wasRolling = m_isPlaying || (m_session && (m_session->transport_rolling() || m_session->transport_speed() != 0.0));

    // 🛑 FRENADO INMEDIATO DE TRANSPORTE
    m_isPlaying = false;
    Q_EMIT isPlayingChanged();

    if (m_session) {
        m_session->request_stop();
    }

    qDebug() << "⏹️ [TRANSPORT] STOP";

    if (wasRolling) {
        Q_EMIT transportStoppedWasRecording();
    }
}

void NovaTransportController::rewind()
{
    locateFrame(0);
    qDebug() << "⏮️ [TRANSPORT] REWIND -> 0";
}

void NovaTransportController::togglePlay()
{
    if (m_isPlaying) {
        stop();
    } else {
        play();
    }
}

void NovaTransportController::setBpm(double bpm)
{
    if (qFuzzyCompare(m_bpm, bpm)) return;
    m_bpm = bpm;
    Q_EMIT bpmChanged();
}

// 🎯 REUBICACIÓN INSTANTÁNEA (SCRUBBING 0ms LAG BLINDADO)
void NovaTransportController::locateFrame(double frame)
{
    if (!m_session) return;
    ARDOUR::samplepos_t pos = static_cast<ARDOUR::samplepos_t>(std::max(0.0, frame));
    
    m_session->request_locate(pos);
    m_targetLocateFrame = static_cast<double>(pos);
    m_locatePending = true;

    m_currentFrame = static_cast<double>(pos);
    ARDOUR::samplecnt_t sr = m_session->sample_rate() > 0 ? m_session->sample_rate() : 44100;

    double currentSeconds = m_currentFrame / static_cast<double>(sr);
    double bpm = m_bpm > 0 ? m_bpm : 120.0;
    m_currentBeat = currentSeconds * (bpm / 60.0);

    int totalSeconds = static_cast<int>(currentSeconds);
    int mins = totalSeconds / 60;
    int secs = totalSeconds % 60;
    int ms = static_cast<int>((m_currentFrame / (static_cast<double>(sr) / 100.0))) % 100;

    m_timecode = QString("%1:%2:%3")
                 .arg(mins, 2, 10, QChar('0'))
                 .arg(secs, 2, 10, QChar('0'))
                 .arg(ms, 2, 10, QChar('0'));

    int bar = 1 + static_cast<int>(m_currentBeat / 4.0);
    int beat = 1 + static_cast<int>(std::fmod(m_currentBeat, 4.0));
    int sub = 1 + static_cast<int>(std::fmod(m_currentBeat * 4.0, 4.0));

    m_bbt = QString("%1 : %2 : %3")
            .arg(bar, 3, 10, QChar('0'))
            .arg(beat, 2, 10, QChar('0'))
            .arg(sub, 2, 10, QChar('0'));

    Q_EMIT timecodeChanged();
    Q_EMIT bbtChanged();
    Q_EMIT positionChanged();
}

void NovaTransportController::locateBeat(double beat)
{
    if (!m_session) return;
    double sr = m_session->sample_rate() > 0 ? static_cast<double>(m_session->sample_rate()) : 44100.0;
    double bpm = m_bpm > 0 ? m_bpm : 120.0;
    double seconds = beat * (60.0 / bpm);
    locateFrame(seconds * sr);
}

void NovaTransportController::setLoopRange(double startBeat, double endBeat)
{
    if (!m_session || !m_session->locations()) return;

    m_loopStartBeat = std::max(0.0, startBeat);
    m_loopEndBeat = std::max(m_loopStartBeat + 1.0, endBeat);

    double sr = m_session->sample_rate() > 0 ? static_cast<double>(m_session->sample_rate()) : 44100.0;
    double bpm = m_bpm > 0 ? m_bpm : 120.0;
    double startSeconds = m_loopStartBeat * (60.0 / bpm);
    double endSeconds = m_loopEndBeat * (60.0 / bpm);

    ARDOUR::samplepos_t startFrame = static_cast<ARDOUR::samplepos_t>(startSeconds * sr);
    ARDOUR::samplepos_t endFrame = static_cast<ARDOUR::samplepos_t>(endSeconds * sr);

    ARDOUR::Location* loopLoc = m_session->locations()->auto_loop_location();
    if (!loopLoc) {
        loopLoc = new ARDOUR::Location(*m_session, Temporal::timepos_t(startFrame), Temporal::timepos_t(endFrame), "Loop", ARDOUR::Location::IsAutoLoop);
        m_session->locations()->add(loopLoc);
        m_session->set_auto_loop_location(loopLoc);
    } else {
        loopLoc->set_start(Temporal::timepos_t(startFrame));
        loopLoc->set_end(Temporal::timepos_t(endFrame));
    }

    Q_EMIT loopRangeChanged();
}

void NovaTransportController::setLoopEnabled(bool enabled)
{
    if (!m_session) return;
    if (m_loopEnabled == enabled) return;

    m_loopEnabled = enabled;
    m_session->request_play_loop(m_loopEnabled);
    Q_EMIT loopEnabledChanged();
}

void NovaTransportController::toggleLoop()
{
    setLoopEnabled(!m_loopEnabled);
}

void NovaTransportController::updatePositionFromArdour()
{
    if (!m_session) return;

    // 🛡️ SI EL USUARIO PULSÓ STOP / PARÓ DE GRABAR: Congelar aguja inmediatamente
    if (!m_isPlaying) {
        m_locatePending = false;
        return;
    }

    ARDOUR::samplecnt_t sr = m_session->sample_rate() > 0 ? m_session->sample_rate() : 44100;
    ARDOUR::samplepos_t pos = m_session->transport_sample();

    // 🚀 SINCRO EN TIEMPO REAL
    if (static_cast<double>(pos) > m_currentFrame) {
        m_currentFrame = static_cast<double>(pos);
    } else if (!m_locatePending) {
        // Avance simulado a 60 FPS (16ms) mientras el transporte esté activo
        m_currentFrame += (static_cast<double>(sr) * 0.016);
    }

    if (m_locatePending) {
        if (std::abs(m_currentFrame - m_targetLocateFrame) < (static_cast<double>(sr) * 0.25)) {
            m_locatePending = false;
        }
    }

    double currentSeconds = m_currentFrame / static_cast<double>(sr);
    double bpm = m_bpm > 0 ? m_bpm : 120.0;
    m_currentBeat = currentSeconds * (bpm / 60.0);

    int totalSeconds = static_cast<int>(currentSeconds);
    int mins = totalSeconds / 60;
    int secs = totalSeconds % 60;
    int ms = static_cast<int>((m_currentFrame / (static_cast<double>(sr) / 100.0))) % 100;

    m_timecode = QString("%1:%2:%3")
                 .arg(mins, 2, 10, QChar('0'))
                 .arg(secs, 2, 10, QChar('0'))
                 .arg(ms, 2, 10, QChar('0'));

    int bar = 1 + static_cast<int>(m_currentBeat / 4.0);
    int beat = 1 + static_cast<int>(std::fmod(m_currentBeat, 4.0));
    int sub = 1 + static_cast<int>(std::fmod(m_currentBeat * 4.0, 4.0));

    m_bbt = QString("%1 : %2 : %3")
            .arg(bar, 3, 10, QChar('0'))
            .arg(beat, 2, 10, QChar('0'))
            .arg(sub, 2, 10, QChar('0'));

    Q_EMIT timecodeChanged();
    Q_EMIT bbtChanged();
    Q_EMIT positionChanged();
}