#include "NovaTrackListModel.h"
#include "core/NovaLogging.h"
#include "core/platform/NovaAndroidStubs.h"

#include <cmath>
#include <algorithm>

#if !defined(Q_OS_ANDROID)
#pragma push_macro("emit")
#pragma push_macro("slots")
#pragma push_macro("signals")
#pragma push_macro("foreach")
#undef emit
#undef slots
#undef signals
#undef foreach

#include "ardour/session.h"
#include "ardour/route.h"
#include "ardour/track.h"
#include "ardour/audio_track.h"
#include "ardour/midi_track.h"
#include "ardour/gain_control.h"
#include "ardour/mute_master.h"
#include "ardour/solo_control.h"
#include "ardour/automation_control.h"
#include "ardour/presentation_info.h"
#include "ardour/types.h"
#include "ardour/meter.h"

#pragma pop_macro("emit")
#pragma pop_macro("slots")
#pragma pop_macro("signals")
#pragma pop_macro("foreach")
#endif

NovaTrackListModel::NovaTrackListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

NovaTrackListModel::~NovaTrackListModel()
{
    m_sessionConnections.drop_connections();
}

float NovaTrackListModel::coeffToDb(float coeff)
{
    if (coeff <= 0.0000001f) return -192.0f;
    return 20.0f * std::log10(coeff);
}

float NovaTrackListModel::dbToCoeff(float dB)
{
    if (dB <= -192.0f) return 0.0f;
    return std::pow(10.0f, dB / 20.0f);
}

void NovaTrackListModel::setSession(ARDOUR::Session *session)
{
    beginResetModel();
    m_routes.clear();
    m_session = session;
    endResetModel();

    if (m_session) {
        m_sessionConnections.drop_connections();
        syncWithArdour();
    }
}

void NovaTrackListModel::syncWithArdour()
{
    if (!m_session) return;

    beginResetModel();
    m_routes.clear();

    auto routeList = m_session->get_routes();
    if (routeList) {
        for (auto &route : *routeList) {
            if (route && route->is_track()) {
                m_routes.push_back(route);
            }
        }
    }
    endResetModel();

    Q_EMIT trackCountChanged(static_cast<int>(m_routes.size()));
}

int NovaTrackListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(m_routes.size());
}

QVariant NovaTrackListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_routes.size()))
        return {};

    const auto &route = m_routes[static_cast<size_t>(index.row())];
    if (!route) return {};

    switch (role) {
    case TrackIdRole:
        return QString::number(index.row() + 1);

    case TrackNameRole:
        return QString::fromStdString(route->name());

    case TrackTypeRole:
        return QStringLiteral("audio");

    case GainRole:
        return 0.0f;

    case MuteRole:
        return false;

    case SoloRole:
        return false;

    case RecEnableRole:
        return false;

    case PanRole:
        return 0.0f;

    case ColorRole:
        return QStringLiteral("#3498db");

    case PeakRole:
        return 0.0f;

    default:
        return {};
    }
}

QHash<int, QByteArray> NovaTrackListModel::roleNames() const
{
    return {
        { TrackIdRole,     "trackId"     },
        { TrackNameRole,   "trackName"   },
        { TrackTypeRole,   "trackType"   },
        { GainRole,        "trackGain"   },
        { MuteRole,        "trackMute"   },
        { SoloRole,        "trackSolo"   },
        { RecEnableRole,   "trackRec"    },
        { PanRole,         "trackPan"    },
        { ColorRole,       "trackColor"  },
        { PeakRole,        "trackPeak"   }
    };
}

void NovaTrackListModel::addAudioTrack(const QString &name)
{
    if (!m_session) return;
    qCDebug(novaModel) << "➕ Creando nueva pista de audio:" << name;
    m_session->set_dirty();
    syncWithArdour();
}

void NovaTrackListModel::addMidiTrack(const QString &name)
{
    if (!m_session) return;
    qCDebug(novaModel) << "➕ Creando nueva pista MIDI:" << name;
    m_session->set_dirty();
    syncWithArdour();
}

void NovaTrackListModel::removeTrack(int row)
{
    if (row < 0 || row >= static_cast<int>(m_routes.size()) || !m_session) return;

    beginRemoveRows(QModelIndex(), row, row);
    m_routes.erase(m_routes.begin() + row);
    endRemoveRows();

    m_session->set_dirty();
    Q_EMIT trackCountChanged(static_cast<int>(m_routes.size()));
}

void NovaTrackListModel::setGain(int row, float dB)
{
    if (row < 0 || row >= static_cast<int>(m_routes.size()) || !m_session) return;
    Q_EMIT dataChanged(index(row), index(row), {GainRole});
}

void NovaTrackListModel::setMute(int row, bool muted)
{
    if (row < 0 || row >= static_cast<int>(m_routes.size()) || !m_session) return;
    Q_EMIT dataChanged(index(row), index(row), {MuteRole});
}

void NovaTrackListModel::setSolo(int row, bool soloed)
{
    if (row < 0 || row >= static_cast<int>(m_routes.size()) || !m_session) return;
    Q_EMIT dataChanged(index(row), index(row), {SoloRole});
}

void NovaTrackListModel::setRecEnable(int row, bool enabled)
{
    if (row < 0 || row >= static_cast<int>(m_routes.size()) || !m_session) return;
    Q_EMIT dataChanged(index(row), index(row), {RecEnableRole});
}

void NovaTrackListModel::setPan(int row, float pan)
{
    if (row < 0 || row >= static_cast<int>(m_routes.size()) || !m_session) return;
    Q_EMIT dataChanged(index(row), index(row), {PanRole});
}

float NovaTrackListModel::peakLevel(int row) const
{
    Q_UNUSED(row);
    return 0.0f;
}