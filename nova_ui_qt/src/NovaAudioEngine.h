#ifndef NOVAAUDIOENGINE_H
#define NOVAAUDIOENGINE_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QDebug>
#include "NovaTrackListModel.h"
#include "NovaRegionModel.h"

namespace ARDOUR {
    class AudioEngine;
    class Session;
}

class NovaAudioEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)
    Q_PROPERTY(bool isRecording READ isRecording NOTIFY isRecordingChanged)
    Q_PROPERTY(QString timecode READ timecode NOTIFY timecodeChanged)
    Q_PROPERTY(QString bbt READ bbt NOTIFY bbtChanged)
    Q_PROPERTY(double currentFrame READ currentFrame NOTIFY positionChanged)
    Q_PROPERTY(double bpm READ bpm WRITE setBpm NOTIFY bpmChanged)
    Q_PROPERTY(NovaTrackListModel* tracks READ tracks NOTIFY tracksChanged)
    Q_PROPERTY(NovaRegionModel* regions READ regions NOTIFY regionsChanged)
    
    // 🔁 PROPIEDADES DE BUCLE Y UBICACIÓN
    Q_PROPERTY(bool loopEnabled READ loopEnabled WRITE setLoopEnabled NOTIFY loopEnabledChanged)
    Q_PROPERTY(double loopStartBeat READ loopStartBeat NOTIFY loopRangeChanged)
    Q_PROPERTY(double loopEndBeat READ loopEndBeat NOTIFY loopRangeChanged)

public:
    explicit NovaAudioEngine(QObject *parent = nullptr);
    ~NovaAudioEngine();

    bool initEngine();

    Q_INVOKABLE void play();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void rewind();
    Q_INVOKABLE void togglePlay();
    Q_INVOKABLE void toggleRecord();
    Q_INVOKABLE void setBpm(double newBpm);

    // 🎯 MÉTODOS DE SCRUBBING Y LOOP (NUEVOS)
    Q_INVOKABLE void locateFrame(double frame);
    Q_INVOKABLE void locateBeat(double beat);
    Q_INVOKABLE void setLoopRange(double startBeat, double endBeat);
    Q_INVOKABLE void setLoopEnabled(bool enabled);
    Q_INVOKABLE void toggleLoop();

    bool isPlaying() const { return m_isPlaying; }
    bool isRecording() const { return m_isRecording; }
    QString timecode() const { return m_timecode; }
    QString bbt() const { return m_bbt; }
    double currentFrame() const { return m_currentFrame; }
    double bpm() const { return m_bpm; }
    NovaTrackListModel* tracks() const { return m_trackModel; }
    NovaRegionModel* regions() const { return m_regionModel; }
    bool loopEnabled() const { return m_loopEnabled; }
    double loopStartBeat() const { return m_loopStartBeat; }
    double loopEndBeat() const { return m_loopEndBeat; }

signals:
    void isPlayingChanged();
    void isRecordingChanged();
    void timecodeChanged();
    void bbtChanged();
    void positionChanged();
    void bpmChanged();
    void tracksChanged();
    void regionsChanged();
    void loopEnabledChanged();
    void loopRangeChanged();

private slots:
    void updatePositionFromArdour();

private:
    bool m_isPlaying = false;
    bool m_isRecording = false;
    QString m_timecode = "00:00:00";
    QString m_bbt = "001 : 01 : 01";
    double m_currentFrame = 0.0;
    double m_bpm = 120.0;

    // 🔁 Variables de Loop
    bool m_loopEnabled = false;
    double m_loopStartBeat = 0.0;   // Compás 1
    double m_loopEndBeat = 16.0;    // Compás 5

    QTimer *m_updateTimer = nullptr;

    ARDOUR::AudioEngine *m_engine = nullptr;
    ARDOUR::Session *m_session = nullptr;
    NovaTrackListModel *m_trackModel = nullptr;
    NovaRegionModel *m_regionModel = nullptr;
};

#endif // NOVAAUDIOENGINE_H