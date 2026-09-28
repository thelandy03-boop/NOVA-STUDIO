#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QByteArray>
#include <QString>
#include <QVariant>
#include <vector>
#include <memory>
#include <mutex>
#include <map>
#include <thread>

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
    QString color = "#4A90E2";
    bool isLiveRecording = false;
    std::shared_ptr<ARDOUR::Region> regionPtr;

    std::vector<PeakPoint> ramPeaks;
};

class NovaRegionModel : public QAbstractListModel
{
    Q_OBJECT

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
        IsLiveRecordingRole
    };
    Q_ENUM(RegionRoles)

    explicit NovaRegionModel(QObject *parent = nullptr);
    ~NovaRegionModel() override;

    void waitForImports();

    void setSession(ARDOUR::Session *session);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE bool importAudioFile(int trackIndex, const QString &filePath, double startBeat = 0.0);
    Q_INVOKABLE bool moveRegion(int regionIndex, double newStartBeat);
    Q_INVOKABLE bool resizeRegion(int regionIndex, double newStartBeat, double newLengthBeats);
    Q_INVOKABLE void removeRegion(int regionIndex);

    // 🎙️ Grabación en vivo
    void createLiveRecordingClip(double startBeat);
    void updateLiveRecordingClip(double lengthBeats);
    void finalizeLiveRecordingClip();

    Q_INVOKABLE void rebuildRegionCache();

    // 🚀 Utilidades de Persistencia .novapeak
    static bool savePeakFile(const QString &peakFilePath, const std::vector<PeakPoint> &peaks);
    static bool loadPeakFile(const QString &peakFilePath, std::vector<PeakPoint> &outPeaks);

signals:
    void regionCountChanged(int count);

private:
    ARDOUR::Session *m_session = nullptr;
    std::vector<NovaRegionItem> m_regions;
    PBD::ScopedConnectionList m_sessionConnections;
    int m_liveRecordingIndex = -1;
    std::vector<std::thread> m_importThreads;
};
