#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QByteArray>
#include <QString>
#include <QVariant>
#include <QtGlobal>
#include <vector>
#include <memory>
#include <mutex>
#include <map>
#include <thread>
#include <atomic>

#if defined(Q_OS_ANDROID) || defined(Q_OS_WIN)
#include "core/platform/NovaAndroidStubs.h"
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
#include "ardour/track.h"
#include "ardour/playlist.h"
#include "ardour/region.h"
#include "pbd/signals.h"

#pragma pop_macro("emit")
#pragma pop_macro("slots")
#pragma pop_macro("signals")
#pragma pop_macro("foreach")
#endif

// 🚀 Estructura de Picos en Memoria RAM
struct PeakPoint {
    float min = 0.0f;
    float max = 0.0f;
    float rms = 0.0f; // 🚀 Energía acumulada RMS para textura HD
};

// 🚀 Sistema Centralizado de Caché de Picos en RAM
class NovaWaveformCache {
public:
    static void setPeaks(const QString &regionId, const std::vector<PeakPoint> &peaks);
    static bool getPeaks(const QString &regionId, std::vector<PeakPoint> &outPeaks);
    static void removePeaks(const QString &regionId); // 🧹 Libera RAM al borrar clip (Bug 6)
    static void clear();

private:
    static inline std::map<QString, std::vector<PeakPoint>> s_cache;
    static inline std::mutex s_mutex;
};

struct NovaRegionItem {
    QString id;
    int trackIndex = 0;
    QString name;
    double startFrame = 0.0;
    double lengthFrames = 0.0;
    double startBeat = 0.0;
    double lengthBeats = 0.0;
    QString color = QStringLiteral("#4A90E2");
    bool isLiveRecording = false;

    // Propiedades de Ganancia y Fundidos (Fades)
    float clipGainDb = 0.0f;
    double fadeInBeats = 0.0;
    double fadeOutBeats = 0.0;

    std::shared_ptr<ARDOUR::Region> regionPtr = nullptr;
};

class NovaRegionModel : public QAbstractListModel
{
    Q_OBJECT
    
    // 🎯 Propiedad de Selección Global del Clip Activo en el Timeline
    Q_PROPERTY(int selectedRegionIndex READ selectedRegionIndex WRITE setSelectedRegionIndex NOTIFY selectedRegionIndexChanged)

public:
    enum RegionRoles {
        RegionIdRole = Qt::UserRole + 1,
        TrackIndexRole,
        RegionNameRole,
        StartFrameRole,
        LengthFramesRole,
        StartBeatRole,
        LengthBeatsRole,
        ColorRole,
        IsLiveRecordingRole,
        ClipGainDbRole,
        FadeInBeatsRole,
        FadeOutBeatsRole
    };
    Q_ENUM(RegionRoles)

    explicit NovaRegionModel(QObject *parent = nullptr);
    ~NovaRegionModel() override;

    void setSession(ARDOUR::Session *session);
    void waitForImports();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Selección de clips
    int selectedRegionIndex() const { return m_selectedRegionIndex; }
    void setSelectedRegionIndex(int index);

    Q_INVOKABLE bool importAudioFile(int trackIndex, const QString &filePath, double startBeat);
    Q_INVOKABLE bool moveRegion(int regionIndex, double newStartBeat);
    Q_INVOKABLE bool resizeRegion(int regionIndex, double newStartBeat, double newLengthBeats);
    Q_INVOKABLE void removeRegion(int regionIndex);

    // ✂️ Herramienta Split (Cortar Clips)
    Q_INVOKABLE bool splitRegion(int regionIndex, double splitBeat);

    // Métodos de Clip Gain, Fades y Normalización
    Q_INVOKABLE void setClipGainDb(int regionIndex, float dB);
    Q_INVOKABLE void setFadeInBeats(int regionIndex, double beats);
    Q_INVOKABLE void setFadeOutBeats(int regionIndex, double beats);
    Q_INVOKABLE void normalizeClip(int regionIndex); // ⚡ Auto-Gain 1-Toque

    // Control de Grabación en Vivo
    void createLiveRecordingClip(double startBeat);
    void updateLiveRecordingClip(double lengthBeats);
    void finalizeLiveRecordingClip();

    // Persistencia y Manejo de Peaks en Disco (.novapeak)
    static bool savePeakFile(const QString &peakFilePath, const std::vector<PeakPoint> &peaks);
    static bool loadPeakFile(const QString &peakFilePath, std::vector<PeakPoint> &outPeaks);

signals:
    void regionCountChanged(int count);
    void selectedRegionIndexChanged(int index);

private:
    void rebuildRegionCache();
    std::shared_ptr<ARDOUR::Track> findTrackByIndex(int trackIndex) const; // 🧹 Clean Code (CC-2)
    void cleanupImportThreads(); // 🧹 Limpieza automática de hilos terminados (Bug 3)

    struct ImportWorker {
        std::thread thread;
        std::shared_ptr<std::atomic<bool>> finished;
    };

    ARDOUR::Session *m_session = nullptr;
    std::vector<NovaRegionItem> m_regions;
    PBD::ScopedConnectionList m_sessionConnections;
    std::vector<ImportWorker> m_importWorkers;
    std::mutex m_workersMutex;

    int m_liveRecordingIndex = -1;
    int m_selectedRegionIndex = -1;
};