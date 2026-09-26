#ifndef NOVAAUDIOENGINE_H
#define NOVAAUDIOENGINE_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QDebug>

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

    bool isPlaying() const { return m_isPlaying; }
    bool isRecording() const { return m_isRecording; }
    QString timecode() const { return m_timecode; }
    QString bbt() const { return m_bbt; }
    double currentFrame() const { return m_currentFrame; }
    double bpm() const { return m_bpm; }

signals:
    void isPlayingChanged();
    void isRecordingChanged();
    void timecodeChanged();
    void bbtChanged();
    void positionChanged();
    void bpmChanged();

private slots:
    void updatePositionFromArdour();

private:
    bool m_isPlaying = false;
    bool m_isRecording = false;
    QString m_timecode = "00:00:00";
    QString m_bbt = "001 : 01 : 01";
    double m_currentFrame = 0.0;
    double m_bpm = 120.0;

    QTimer *m_updateTimer = nullptr;

    ARDOUR::AudioEngine *m_engine = nullptr;
    ARDOUR::Session *m_session = nullptr;
};

#endif // NOVAAUDIOENGINE_H