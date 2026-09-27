#include "NovaTrackListModel.h"
#include <QDebug>
#include <algorithm>
#include <cmath>

#pragma push_macro("emit")
#pragma push_macro("slots")
#pragma push_macro("signals")
#pragma push_macro("foreach")
#undef emit
#undef slots
#undef signals
#undef foreach

#include "ardour/audioengine.h"
#include "ardour/audio_track.h"
#include "ardour/midi_track.h"
#include "ardour/gain_control.h"
#include "ardour/mute_master.h"
#include "ardour/solo_control.h"
#include "ardour/automation_control.h"
#include "ardour/presentation_info.h"
#include "ardour/types.h"

#pragma pop_macro("emit")
#pragma pop_macro("slots")
#pragma pop_macro("signals")
#pragma pop_macro("foreach")

NovaTrackListModel::NovaTrackListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

NovaTrackListModel::~NovaTrackListModel()
{
    disconnectSessionSignals();
}

void NovaTrackListModel::setSession(ARDOUR::Session *session)
{
    if (m_session == session) return;

    disconnectSessionSignals();

    beginResetModel();
    m_routes.clear();
    m_session = session;
    endResetModel();

    if (m_session) {
        rebuildRouteCache();
        connectSessionSignals();
    }

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
        return QString::fromStdString(route->id().to_s());

    case TrackNameRole:
        return QString::fromStdString(route->name());

    case TrackTypeRole:
        if (std::dynamic_pointer_cast<ARDOUR::AudioTrack>(route)) return QStringLiteral("audio");
        if (std::dynamic_pointer_cast<ARDOUR::MidiTrack>(route)) return QStringLiteral("midi");
        return QStringLiteral("bus");

    case GainRole: {
        auto gc = route->gain_control();
        if (gc) return coeffToDb(static_cast<float>(gc->get_value()));
        return 0.0f;
    }

    case MuteRole: {
        auto mc = route->mute_control();
        if (mc) return mc->get_value() != 0.0f;
        return false;
    }

    case SoloRole: {
        auto sc = route->solo_control();
        if (sc) return sc->get_value() != 0.0f;
        return false;
    }

    case RecEnableRole: {
        auto track = std::dynamic_pointer_cast<ARDOUR::Track>(route);
        if (track) {
            auto rec = track->rec_enable_control();
            if (rec) return rec->get_value() != 0.0f;
        }
        return false;
    }

    case PanRole: {
        auto panControl = route->pan_azimuth_control();
        if (panControl) {
            float ardourPan = static_cast<float>(panControl->get_value());
            return (ardourPan - 0.5f) * 2.0f;
        }
        return 0.0f;
    }

    case ColorRole:
        return QStringLiteral("#3498db");

    default:
        return {};
    }
}

QHash<int, QByteArray> NovaTrackListModel::roleNames() const
{
    return {
        { TrackIdRole, "trackId" },
        { TrackNameRole, "trackName" },
        { TrackTypeRole, "trackType" },
        { GainRole, "gain" },
        { MuteRole, "mute" },
        { SoloRole, "solo" },
        { RecEnableRole, "recEnable" },
        { PanRole, "pan" },
        { ColorRole, "trackColor" }
    };
}

void NovaTrackListModel::addAudioTrack(const QString &name)
{
    if (!m_session) {
        qWarning() << "❌ [NovaTracks] Imposible crear pista: No hay sesión de Ardour activa.";
        return;
    }

    qDebug() << "⚡ [NovaTracks] Creando pista Audio Stereo en Ardour Core:" << name;

    ARDOUR::RouteList routes;
    ARDOUR::AudioTrackList tracks;

    // API C++ nativa y estable para creación de AudioTracks
    bool ok = m_session->new_audio_routes_tracks_bulk(
        routes, tracks,
        2, 2,                                // 2 Entradas / 2 Salidas (Stereo)
        nullptr,                             // RouteGroup (ninguno)
        1,                                   // Cantidad (1 pista)
        name.toStdString(),                  // Nombre
        ARDOUR::PresentationInfo::max_order, // Posición en orden
        ARDOUR::Normal,                      // Modo de pista Normal
        true,                                // Autoconectar entradas
        false                                // Trigger visibility
    );

    if (ok && !routes.empty()) {
        m_session->add_routes(routes, true, true, ARDOUR::PresentationInfo::max_order);
        qDebug() << "✅ [NovaTracks] Pista de audio registrada exitosamente en la sesión. Total:" << routes.size();
    } else {
        qWarning() << "❌ [NovaTracks] new_audio_routes_tracks_bulk devolvió un resultado fallido.";
    }
}

void NovaTrackListModel::addMidiTrack(const QString &name)
{
    if (!m_session) return;

    qDebug() << "⚡ [NovaTracks] Creando pista MIDI en Ardour Core:" << name;

    ARDOUR::RouteList rl = m_session->new_midi_route(
        nullptr,
        1,
        name.toStdString(),
        true,
        nullptr,
        nullptr,
        ARDOUR::PresentationInfo::MidiTrack,
        ARDOUR::PresentationInfo::max_order
    );

    if (!rl.empty()) {
        m_session->add_routes(rl, true, true, ARDOUR::PresentationInfo::max_order);
        qDebug() << "✅ [NovaTracks] Pista MIDI registrada exitosamente en la sesión.";
    } else {
        qWarning() << "❌ [NovaTracks] new_midi_route devolvió una lista vacía.";
    }
}

void NovaTrackListModel::removeTrack(int row)
{
    if (!m_session || row < 0 || row >= static_cast<int>(m_routes.size())) return;

    auto route = m_routes[static_cast<size_t>(row)];

    beginRemoveRows(QModelIndex(), row, row);
    m_routes.erase(m_routes.begin() + row);
    endRemoveRows();

    m_session->remove_route(route);
    Q_EMIT trackCountChanged(static_cast<int>(m_routes.size()));
    qDebug() << "🗑️ [NovaTracks] Pista eliminada:" << QString::fromStdString(route->name());
}

void NovaTrackListModel::setGain(int row, float dB)
{
    if (row < 0 || row >= static_cast<int>(m_routes.size())) return;
    auto gc = m_routes[static_cast<size_t>(row)]->gain_control();
    if (gc) {
        gc->set_value(dbToCoeff(dB), PBD::Controllable::NoGroup);
        Q_EMIT dataChanged(index(row), index(row), {GainRole});
    }
}

void NovaTrackListModel::setMute(int row, bool muted)
{
    if (row < 0 || row >= static_cast<int>(m_routes.size())) return;
    auto mc = m_routes[static_cast<size_t>(row)]->mute_control();
    if (mc) {
        mc->set_value(muted ? 1.0f : 0.0f, PBD::Controllable::NoGroup);
        Q_EMIT dataChanged(index(row), index(row), {MuteRole});
    }
}

void NovaTrackListModel::setSolo(int row, bool soloed)
{
    if (row < 0 || row >= static_cast<int>(m_routes.size())) return;
    auto sc = m_routes[static_cast<size_t>(row)]->solo_control();
    if (sc) {
        sc->set_value(soloed ? 1.0f : 0.0f, PBD::Controllable::NoGroup);
        Q_EMIT dataChanged(index(row), index(row), {SoloRole});
    }
}

void NovaTrackListModel::setRecEnable(int row, bool enabled)
{
    if (row < 0 || row >= static_cast<int>(m_routes.size())) return;
    auto track = std::dynamic_pointer_cast<ARDOUR::Track>(m_routes[static_cast<size_t>(row)]);
    if (track) {
        auto rec = track->rec_enable_control();
        if (rec) {
            rec->set_value(enabled ? 1.0f : 0.0f, PBD::Controllable::NoGroup);
            Q_EMIT dataChanged(index(row), index(row), {RecEnableRole});
        }
    }
}

void NovaTrackListModel::setPan(int row, float pan)
{
    if (row < 0 || row >= static_cast<int>(m_routes.size())) return;
    auto panControl = m_routes[static_cast<size_t>(row)]->pan_azimuth_control();
    if (panControl) {
        double ardourPan = std::clamp((static_cast<double>(pan) + 1.0) / 2.0, 0.0, 1.0);
        panControl->set_value(ardourPan, PBD::Controllable::NoGroup);
        Q_EMIT dataChanged(index(row), index(row), {PanRole});
    }
}

void NovaTrackListModel::connectSessionSignals()
{
    if (!m_session) return;

    m_session->RouteAdded.connect_same_thread(m_sessionConnections, [this](ARDOUR::RouteList &routes) {
        for (auto &route : routes) {
            if (route) {
                QMetaObject::invokeMethod(this, [this, route]() {
                    onRouteAdded(route);
                }, Qt::QueuedConnection);
            }
        }
    });
}

void NovaTrackListModel::disconnectSessionSignals()
{
    m_sessionConnections.drop_connections();
}

void NovaTrackListModel::onRouteAdded(std::shared_ptr<ARDOUR::Route> route)
{
    if (!route || !route->is_track()) return;

    auto it = std::find(m_routes.begin(), m_routes.end(), route);
    if (it != m_routes.end()) return;

    int row = static_cast<int>(m_routes.size());
    beginInsertRows(QModelIndex(), row, row);
    m_routes.push_back(route);
    endInsertRows();

    Q_EMIT trackCountChanged(static_cast<int>(m_routes.size()));
    qDebug() << "🎯 [NovaTracks] Nueva pista agregada al modelo QML:" << QString::fromStdString(route->name());
}

void NovaTrackListModel::onRouteRemoved(std::shared_ptr<ARDOUR::Route> route)
{
    auto it = std::find(m_routes.begin(), m_routes.end(), route);
    if (it == m_routes.end()) return;

    int row = static_cast<int>(std::distance(m_routes.begin(), it));
    beginRemoveRows(QModelIndex(), row, row);
    m_routes.erase(it);
    endRemoveRows();

    Q_EMIT trackCountChanged(static_cast<int>(m_routes.size()));
    qDebug() << "🗑️ [NovaTracks] Pista eliminada del modelo QML:" << QString::fromStdString(route->name());
}

void NovaTrackListModel::rebuildRouteCache()
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
}

float NovaTrackListModel::coeffToDb(float coeff)
{
    if (coeff <= 0.0f) return -192.0f;
    return 20.0f * std::log10(coeff);
}

float NovaTrackListModel::dbToCoeff(float dB)
{
    if (dB <= -192.0f) return 0.0f;
    return std::pow(10.0f, dB / 20.0f);
}