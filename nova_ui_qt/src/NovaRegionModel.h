#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QByteArray>
#include <QString>
#include <QVariant>
#include <QMetaObject>
#include <vector>
#include <memory>

// ── PROTECCIÓN CONTRA COLISIONES DE SEÑALES ARDOUR/QT ────────────────
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
// ─────────────────────────────────────────────────────────────────────

struct NovaRegionItem {
    QString id;
    int trackIndex;
    QString name;
    double startFrame;
    double lengthFrames;
    double startBeat;
    double lengthBeats;
    QString color;
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
        ColorRole
    };
    Q_ENUM(RegionRoles)

    explicit NovaRegionModel(QObject *parent = nullptr);
    ~NovaRegionModel() override;

    void setSession(ARDOUR::Session *session);

    // QAbstractListModel overrides
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // API pública invocable desde QML
    Q_INVOKABLE bool importAudioFile(int trackIndex, const QString &filePath, double startBeat = 0.0);
    Q_INVOKABLE void removeRegion(int regionIndex);
    Q_INVOKABLE void rebuildRegionCache();

signals:
    void regionCountChanged(int count);

private:
    ARDOUR::Session *m_session = nullptr;
    std::vector<NovaRegionItem> m_regions;
    PBD::ScopedConnectionList m_sessionConnections;
};