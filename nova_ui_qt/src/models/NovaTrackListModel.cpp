#include "NovaTrackListModel.h"
#include <QDebug>
#include <cmath>
#include <algorithm>

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

// 🎛️ SOFT CLIPPER ANALÓGICO TRANSPARENTE
inline float applySoftClip(float x) noexcept
{
    constexpr float threshold = 0.8f;
    constexpr float margin = 0.2f;

    if (x > threshold) {
        return threshold + margin * std::tanh((x - threshold) / margin);
    } else if (x < -threshold) {
        return -(threshold + margin * std::tanh((-x - threshold) / margin));
    }
    return x;
}

NovaTrackListModel::NovaTrackListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

NovaTrackListModel::~NovaTrackListModel()
{
    m_sessionConnections.drop_connections();
}

void NovaTrackListModel::setSession(ARDOUR::Session *session)
{
    beginResetModel();
    m_routes.clear();
    m_session = session;
    endResetModel();

    if (m_session) {
        m_sessionConnections.drop_connections();

        m_session->RouteAdded.connect_same_thread(
            m_sessionConnections,
            [this](ARDOUR::RouteList & /*routes*/) {
                syncWithArdour();
            }
        );

        m_session->InstrumentRouteAdded.connect_same_thread(
            m_sessionConnections,
            [this](ARDOUR::RouteList & /*routes*/) {
                syncWithArdour();
            }
        );

        syncWithArdour();
    }
}

void NovaTrackListModel::syncWithArdour()
{
    if (!m_session) return;
    auto routeList = m_session->get_routes();

    beginResetModel();
    m_routes.clear();
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
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_routes.size())) return {};
    auto route = m_routes[static_cast<size_t>(index.row())];
    if (!route) return {};

    switch (role) {
    case TrackNameRole: return QString::fromStdString(route->name());
    case TrackTypeRole:
        if (std::dynamic_pointer_cast<ARDOUR::AudioTrack>(route)) return QStringLiteral("audio");
        if (std::dynamic_pointer_cast<ARDOUR::MidiTrack>(route)) return QStringLiteral("midi");
        return QStringLiteral("bus");
    case GainRole: {
        auto gc = route->gain_control();
        if (gc) return coeffToDb(static_cast<float>(gc->get_value()));
        return 0.0f;
    }
    case PanRole: {
        auto panCtrl = route->pan_azimuth_control();
        if (panCtrl) {
            float val = static_cast<float>(panCtrl->get_value());
            return (val * 2.0f) - 1.0f; // Ardour [0.0, 1.0] -> UI [-1.0, 1.0]
        }
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
        if (track && track->rec_enable_control()) return track->rec_enable_control()->get_value() != 0.0f;
        return false;
    }
    case PeakRole: {
        return peakLevel(index.row());
    }
    default: return {};
    }
}

QHash<int, QByteArray> NovaTrackListModel::roleNames() const
{
    return {
        { TrackNameRole, "trackName"   },
        { TrackTypeRole, "trackType"   },
        { GainRole,      "gain"        },
        { PanRole,       "pan"         },
        { MuteRole,      "mute"        },
        { SoloRole,      "solo"        },
        { RecEnableRole, "recEnable"   },
        { PeakRole,      "peakLevel"   }
    };
}

void NovaTrackListModel::addAudioTrack(const QString &name)
{
    if (!m_session) return;

    ARDOUR::RouteList routes;
    ARDOUR::AudioTrackList tracks;

    bool ok = m_session->new_audio_routes_tracks_bulk(
        routes, tracks,
        1, 2,                                
        nullptr, 1, name.toStdString(),
        ARDOUR::PresentationInfo::max_order, ARDOUR::Normal, true, false
    );

    if (ok && !routes.empty()) {
        m_session->add_routes(routes, true, true, ARDOUR::PresentationInfo::max_order);
        
        for (auto &route : routes) {
            if (route) {
                if (auto track = std::dynamic_pointer_cast<ARDOUR::Track>(route)) {
                    track->ensure_input_monitoring(false);
                }
                // 🎛️ Headroom de estudio preventivo (-3 dBFS / 0.707f) estilo FL Studio
                if (route->gain_control()) {
                    route->gain_control()->set_value(0.7079f, PBD::Controllable::NoGroup);
                }
            }
        }
    }
}

void NovaTrackListModel::addMidiTrack(const QString &name)
{
    if (!m_session) return;
    ARDOUR::RouteList rl = m_session->new_midi_route(nullptr, 1, name.toStdString(), true, nullptr, nullptr, ARDOUR::PresentationInfo::MidiTrack, ARDOUR::PresentationInfo::max_order);
    if (!rl.empty()) m_session->add_routes(rl, true, true, ARDOUR::PresentationInfo::max_order);
}

void NovaTrackListModel::removeTrack(int row)
{
    if (row < 0 || row >= static_cast<int>(m_routes.size())) return;
    auto route = m_routes[static_cast<size_t>(row)];
    if (route && m_session) {
        auto rl = std::make_shared<ARDOUR::RouteList>();
        rl->push_back(route);
        m_session->remove_routes(rl);
        syncWithArdour();
    }
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

void NovaTrackListModel::setPan(int row, float pan)
{
    if (row < 0 || row >= static_cast<int>(m_routes.size())) return;
    auto panCtrl = m_routes[static_cast<size_t>(row)]->pan_azimuth_control();
    if (panCtrl) {
        float ardourPan = std::clamp((pan + 1.0f) * 0.5f, 0.0f, 1.0f);
        panCtrl->set_value(ardourPan, PBD::Controllable::NoGroup);
        Q_EMIT dataChanged(index(row), index(row), {PanRole});
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

void NovaTrackListModel::setRecEnable(int row, bool armed)
{
    if (row < 0 || row >= static_cast<int>(m_routes.size())) return;
    auto track = std::dynamic_pointer_cast<ARDOUR::Track>(m_routes[static_cast<size_t>(row)]);
    if (track && track->rec_enable_control()) {
        track->rec_enable_control()->set_value(armed ? 1.0f : 0.0f, PBD::Controllable::NoGroup);
        Q_EMIT dataChanged(index(row), index(row), {RecEnableRole});
    }
}

float NovaTrackListModel::peakLevel(int row) const
{
    if (row < 0 || row >= static_cast<int>(m_routes.size())) return 0.0f;
    auto route = m_routes[static_cast<size_t>(row)];
    if (!route) return 0.0f;

    auto meter = route->peak_meter();
    if (!meter) return 0.0f;

    float val = meter->meter_level(0, ARDOUR::MeterPeak);
    if (std::isnan(val) || std::isinf(val) || val < 0.0f) {
        return 0.0f;
    }
    return applySoftClip(val);
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