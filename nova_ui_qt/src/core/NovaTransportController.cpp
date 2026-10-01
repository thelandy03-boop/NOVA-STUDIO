#include "NovaTransportController.h"
#include "NovaLogging.h"
#include <QDebug>
#include <cmath>
#include <algorithm>

#if defined(Q_OS_ANDROID)
#include "platform/NovaAndroidStubs.h"
#else
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
#endif

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
        qCDebug(novaTransport) << "Sesión vinculada, timer 60 FPS activo.";
    } else {
        m_updateTimer->stop();
    }
}

void NovaTransportController::play()
{
    if (!m_session) return;

    m_locatePending = false;
#if !defined(Q_OS_ANDROID)
    m_session->request_roll();
#endif
    m_isPlaying = true;
    Q_EMIT isPlayingChanged();
    qCDebug(novaTransport) << "ROLL solicitado.";
}

void NovaTransportController::stop()
{
#if defined(Q_OS_ANDROID)
    bool wasRolling = m_isPlaying;
#else
    bool wasRolling = m_isPlaying || (m_session && (m_session->transport_rolling() || m_session->transport_speed() != 0.0));
#endif

    m_isPlaying = false;
    Q_EMIT isPlayingChanged();

#if !defined(Q_OS_ANDROID)
    if (m_session) {
        m_session->request_stop();
    }
#endif

    qCDebug(novaTransport) << "STOP solicitado.";

    if (wasRolling) {
        Q_EMIT transportStoppedWasRecording();
    }
}

void NovaTransportController::rewind()
{
    locateFrame(0);
    qCDebug(novaTransport) << "REWIND a compás 1.";
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

void NovaTransportController::locateFrame(double frame)
{
    if (!m_session) return;
    ARDOUR::samplepos_t pos = static_cast<ARDOUR::samplepos_t>(std::max(0.0, frame));
    
#if !defined(Q_OS_ANDROID)
    m_session->request_locate(pos);
#endif
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
    m_loopStartBeat = std::max(0.0, startBeat);
    m_loopEndBeat = std::max(m_loopStartBeat + 1.0, endBeat);

#if !defined(Q_OS_ANDROID)
    if (!m_session || !m_session->locations()) return;

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
#endif

    Q_EMIT loopRangeChanged();
}

void NovaTransportController::setLoopEnabled(bool enabled)
{
    if (!m_session) return;
    if (m_loopEnabled == enabled) return;

    m_loopEnabled = enabled;
#if !defined(Q_OS_ANDROID)
    m_session->request_play_loop(m_loopEnabled);
#endif
    Q_EMIT loopEnabledChanged();
}

void NovaTransportController::toggleLoop()
{
    setLoopEnabled(!m_loopEnabled);
}

void NovaTransportController::updatePositionFromArdour()
{
    if (!m_session) return;

    if (!m_isPlaying) {
        m_locatePending = false;
        return;
    }

    ARDOUR::samplecnt_t sr = m_session->sample_rate() > 0 ? m_session->sample_rate() : 44100;

#if defined(Q_OS_ANDROID)
    m_currentFrame += (static_cast<double>(sr) * 0.016);
#else
    ARDOUR::samplepos_t pos = m_session->transport_sample();

    if (static_cast<double>(pos) > m_currentFrame) {
        m_currentFrame = static_cast<double>(pos);
    } else if (!m_locatePending) {
        m_currentFrame += (static_cast<double>(sr) * 0.016);
    }

    if (m_locatePending) {
        if (std::abs(m_currentFrame - m_targetLocateFrame) < (static_cast<double>(sr) * 0.25)) {
            m_locatePending = false;
        }
    }
#endif

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