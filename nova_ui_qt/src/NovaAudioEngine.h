#ifndef NOVAAUDIOENGINE_H
#define NOVAAUDIOENGINE_H

#include <QObject>
#include <QString>
#include <QVariantList>

#include "core/NovaSessionManager.h"
#include "core/NovaTransportController.h"
#include "core/NovaRecordManager.h"
#include "core/NovaAudioDeviceManager.h"
#include "core/NovaTimeUtils.h"
#include "models/NovaTrackListModel.h"
#include "models/NovaRegionModel.h"

class NovaAudioEngine : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)
    Q_PROPERTY(bool isRecording READ isRecording NOTIFY isRecordingChanged)
    Q_PROPERTY(QString timecode READ timecode NOTIFY timecodeChanged)
    Q_PROPERTY(QString bbt READ bbt NOTIFY bbtChanged)
    Q_PROPERTY(double currentFrame READ currentFrame NOTIFY positionChanged)
    Q_PROPERTY(double currentBeat READ currentBeat NOTIFY positionChanged)
    Q_PROPERTY(double bpm READ bpm WRITE setBpm NOTIFY bpmChanged)
    
    Q_PROPERTY(NovaTrackListModel* tracks READ tracks NOTIFY tracksChanged)
    Q_PROPERTY(NovaRegionModel* regions READ regions NOTIFY regionsChanged)
    Q_PROPERTY(NovaAudioDeviceManager* deviceManager READ deviceManager CONSTANT)

    Q_PROPERTY(bool loopEnabled READ loopEnabled WRITE setLoopEnabled NOTIFY loopEnabledChanged)
    Q_PROPERTY(double loopStartBeat READ loopStartBeat NOTIFY loopRangeChanged)
    Q_PROPERTY(double loopEndBeat READ loopEndBeat NOTIFY loopRangeChanged)

    // 🎛️ MEDIDOR MASTER ESTÉREO FL STUDIO
    Q_PROPERTY(float masterPeakLeft READ masterPeakLeft NOTIFY masterPeaksChanged)
    Q_PROPERTY(float masterPeakRight READ masterPeakRight NOTIFY masterPeaksChanged)
    Q_PROPERTY(float masterVolumeDb READ masterVolumeDb WRITE setMasterVolumeDb NOTIFY masterVolumeDbChanged)

    // 💾 GESTIÓN DE PROYECTOS Y SESIONES
    Q_PROPERTY(bool isDirty READ isDirty NOTIFY isDirtyChanged)
    Q_PROPERTY(QVariantList recentProjects READ recentProjects NOTIFY recentProjectsChanged)
    Q_PROPERTY(QString currentProjectName READ currentProjectName NOTIFY currentProjectChanged)
    Q_PROPERTY(QString currentProjectPath READ currentProjectName NOTIFY currentProjectChanged)

public:
    explicit NovaAudioEngine(QObject *parent = nullptr);
    ~NovaAudioEngine() override;

    bool initEngine();

    Q_INVOKABLE double beatToPixel(double beat, double barWidth = 80.0) const { return NovaTimeUtils::beatToPixel(beat, barWidth); }
    Q_INVOKABLE double pixelToBeat(double pixelX, double barWidth = 80.0) const { return NovaTimeUtils::pixelToBeat(pixelX, barWidth); }

    // Control de Transporte básico
    Q_INVOKABLE void play() { m_transport.play(); }
    Q_INVOKABLE void stop() { m_transport.stop(); }
    Q_INVOKABLE void rewind() { m_transport.rewind(); }
    Q_INVOKABLE void togglePlay() { m_transport.togglePlay(); }
    Q_INVOKABLE void toggleRecord() { m_recorder.toggleRecord(); }
    Q_INVOKABLE void setBpm(double newBpm) { m_transport.setBpm(newBpm); }

    Q_INVOKABLE void locateFrame(double frame) { m_transport.locateFrame(frame); }
    Q_INVOKABLE void locateBeat(double beat) { m_transport.locateBeat(beat); }
    Q_INVOKABLE void setLoopRange(double startBeat, double endBeat) { m_transport.setLoopRange(startBeat, endBeat); }
    Q_INVOKABLE void setLoopEnabled(bool enabled) { m_transport.setLoopEnabled(enabled); }
    Q_INVOKABLE void toggleLoop() { m_transport.toggleLoop(); }

    // 💾 MÉTODOS DE PROYECTOS EXPUESTOS A QML
    Q_INVOKABLE bool saveProject();
    Q_INVOKABLE bool openProject(const QString &path);
    Q_INVOKABLE bool newProject(const QString &name, const QString &parentDir);
    Q_INVOKABLE void closeProject();

    // Getters
    bool isPlaying() const { return m_transport.isPlaying(); }
    bool isRecording() const { return m_recorder.isRecording(); }
    QString timecode() const { return m_transport.timecode(); }
    QString bbt() const { return m_transport.bbt(); }
    double currentFrame() const { return m_transport.currentFrame(); }
    double currentBeat() const { return m_transport.currentBeat(); }
    double bpm() const { return m_transport.bpm(); }
    
    NovaTrackListModel* tracks() const { return m_trackModel; }
    NovaRegionModel* regions() const { return m_regionModel; }
    NovaAudioDeviceManager* deviceManager() { return &m_deviceManager; }

    bool loopEnabled() const { return m_transport.loopEnabled(); }
    double loopStartBeat() const { return m_transport.loopStartBeat(); }
    double loopEndBeat() const { return m_transport.loopEndBeat(); }

    // Master faders y medidores
    float masterPeakLeft() const;
    float masterPeakRight() const;
    float masterVolumeDb() const;
    Q_INVOKABLE void setMasterVolumeDb(float dB);

    // Getters de Sesión / Proyectos
    bool isDirty() const { return m_sessionManager.isDirty(); }
    QVariantList recentProjects() const { return m_sessionManager.getRecentProjects(); }
    QString currentProjectName() const;
    QString currentProjectPath() const;

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
    void masterPeaksChanged();
    void masterVolumeDbChanged();

    // Señales de guardado y carga
    void isDirtyChanged();
    void recentProjectsChanged();
    void currentProjectChanged();

private:
    NovaSessionManager m_sessionManager;
    NovaTransportController m_transport;
    NovaRecordManager m_recorder;
    NovaAudioDeviceManager m_deviceManager;

    NovaTrackListModel *m_trackModel = nullptr;
    NovaRegionModel *m_regionModel = nullptr;

    void connectSessionSignals();
};

#endif // NOVAAUDIOENGINE_H