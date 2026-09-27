#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QByteArray>
#include <QString>
#include <QVariant>
#include <vector>
#include <memory>

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

struct NovaRegionItem {
    QString id;
    int trackIndex;
    QString name;
    double startFrame;
    double lengthFrames;
    double startBeat;
    double lengthBeats;
    QString color;
    bool isLiveRecording = false;
    std::shared_ptr<ARDOUR::Region> regionPtr;
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

    void setSession(ARDOUR::Session *session);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE bool importAudioFile(int trackIndex, const QString &filePath, double startBeat = 0.0);
    Q_INVOKABLE bool moveRegion(int regionIndex, double newStartBeat);
    Q_INVOKABLE bool resizeRegion(int regionIndex, double newStartBeat, double newLengthBeats);
    Q_INVOKABLE void removeRegion(int regionIndex);

    // 🎙️ MÉTODOS DE GRABACIÓN EN VIVO (NUEVO)
    void createLiveRecordingClip(double startBeat);
    void updateLiveRecordingClip(double lengthBeats);
    void finalizeLiveRecordingClip();

    Q_INVOKABLE void rebuildRegionCache();

signals:
    void regionCountChanged(int count);

private:
    ARDOUR::Session *m_session = nullptr;
    std::vector<NovaRegionItem> m_regions;
    PBD::ScopedConnectionList m_sessionConnections;
    int m_liveRecordingIndex = -1;
};