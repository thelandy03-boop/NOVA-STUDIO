#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

// ── PROTECCIÓN CONTRA COLISIONES ─────────────────────────────────────
#pragma push_macro("emit")
#pragma push_macro("slots")
#pragma push_macro("signals")
#pragma push_macro("foreach")
#undef emit
#undef slots
#undef signals
#undef foreach

namespace ARDOUR {
    class Session;
}

#pragma pop_macro("emit")
#pragma pop_macro("slots")
#pragma pop_macro("signals")
#pragma pop_macro("foreach")

class NovaTransportController : public QObject
{
    Q_OBJECT

public:
    explicit NovaTransportController(QObject *parent = nullptr);
    ~NovaTransportController() override = default;

    void setSession(ARDOUR::Session *session);

    void play();
    void stop();
    void rewind();
    void togglePlay();
    void locateFrame(double frame);
    void locateBeat(double beat);
    void setBpm(double bpm);

    void setLoopRange(double startBeat, double endBeat);
    void setLoopEnabled(bool enabled);
    void toggleLoop();

    bool isPlaying() const { return m_isPlaying; }
    QString timecode() const { return m_timecode; }
    QString bbt() const { return m_bbt; }
    double currentFrame() const { return m_currentFrame; }
    double currentBeat() const { return m_currentBeat; }
    double bpm() const { return m_bpm; }
    bool loopEnabled() const { return m_loopEnabled; }
    double loopStartBeat() const { return m_loopStartBeat; }
    double loopEndBeat() const { return m_loopEndBeat; }

signals:
    void isPlayingChanged();
    void timecodeChanged();
    void bbtChanged();
    void positionChanged();
    void bpmChanged();
    void loopEnabledChanged();
    void loopRangeChanged();
    void transportStoppedWasRecording();

private slots:
    void updatePositionFromArdour();

private:
    ARDOUR::Session *m_session = nullptr;
    QTimer *m_updateTimer = nullptr;

    bool m_isPlaying = false;
    QString m_timecode = "00:00:00";
    QString m_bbt = "001 : 01 : 01";
    double m_currentFrame = 0.0;
    double m_currentBeat = 0.0;
    double m_bpm = 120.0;

    bool m_loopEnabled = false;
    double m_loopStartBeat = 0.0;
    double m_loopEndBeat = 16.0;

    // 🔒 Blindaje contra rebotes de timer
    bool m_locatePending = false;
    double m_targetLocateFrame = 0.0;
};