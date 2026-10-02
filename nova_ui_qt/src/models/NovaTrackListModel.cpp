#include "NovaTrackListModel.h"
#include "core/NovaLogging.h"
#include "core/NovaAudioUtils.h"
#include "core/platform/NovaAndroidStubs.h"

#include <cmath>
#include <algorithm>

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_WIN)
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
#include "ardour/mute_control.h"
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

void NovaTrackListModel::setSession(ARDOUR::Session *session)
{
    if (m_session == session) return;

    m_sessionConnections.drop_connections();
    m_session = session;

    if (m_session) {
        syncWithArdour();
    } else {
        beginResetModel();
        m_routes.clear();
        endResetModel();
        Q_EMIT trackCountChanged(0);
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

    case TrackTypeRole: {
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_WIN)
        auto midiTrk = std::dynamic_pointer_cast<ARDOUR::MidiTrack>(route);
        return midiTrk ? QStringLiteral("midi") : QStringLiteral("audio");
#else
        return QStringLiteral("audio");
#endif
    }

    case GainRole: {
        auto gc = route->gain_control();
        if (gc) {
            float coeff = static_cast<float>(gc->get_value());
            return NovaAudioUtils::coeffToDb(coeff);
        }
        return 0.0f;
    }

    case MuteRole: {
        auto mc = route->mute_control();
        if (mc) {
            return mc->muted();
        }
        return false;
    }

    case SoloRole: {
        auto sc = route->solo_control();
        if (sc) {
            return sc->self_soloed();
        }
        return false;
    }

    case RecEnableRole: {
        auto track = std::dynamic_pointer_cast<ARDOUR::Track>(route);
        if (track) {
            auto rc = track->rec_enable_control();
            if (rc) {
                return (rc->get_value() > 0.5);
            }
        }
        return false;
    }

    case PanRole: {
        auto pc = route->pan_azimuth_control();
        if (pc) {
            // Mapear el azimuth estéreo nativo [0.0 (L) a 1.0 (R)] al formato de la GUI [-1.0 a +1.0]
            float azimuth = static_cast<float>(pc->get_value());
            return (azimuth * 2.0f) - 1.0f;
        }
        return 0.0f; // Centro por defecto
    }

    case ColorRole:
        return QStringLiteral("#3498db");

    case PeakRole:
        return peakLevel(index.row());

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
        { GainRole,        "gain"        },
        { MuteRole,        "mute"        },
        { SoloRole,        "solo"        },
        { RecEnableRole,   "recEnable"   },
        { PanRole,         "pan"         },
        { ColorRole,       "trackColor"  },
        { PeakRole,        "trackPeak"   }
    };
}

void NovaTrackListModel::addAudioTrack(const QString &name)
{
    if (!m_session) return;
    qCDebug(novaModel) << "➕ Creando nueva pista de audio nativa:" << name;

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_WIN)
    try {
        m_session->new_audio_track(
            2,                                     // Canales de entrada
            2,                                     // Canales de salida
            std::shared_ptr<ARDOUR::RouteGroup>(), // Sin grupo de rutas predefinido
            1,                                     // Cantidad de pistas a crear
            name.toStdString(),                    // Nombre de plantilla
            ARDOUR::PresentationInfo::max_order,   // Posicionamiento de orden máximo
            ARDOUR::Normal                         // Modo de pista normal (no destructivo/layered)
        );
        m_session->set_dirty();
    } catch (const std::exception &e) {
        qCWarning(novaModel) << "❌ Error crítico al instanciar pista de audio nativa:" << e.what();
    }
#else
    m_session->new_audio_track(
        2, 2, nullptr, 1, name.toStdString(), ARDOUR::PresentationInfo::max_order, ARDOUR::Normal
    );
#endif

    syncWithArdour();
}

void NovaTrackListModel::addMidiTrack(const QString &name)
{
    if (!m_session) return;
    qCDebug(novaModel) << "➕ Creando nueva pista MIDI nativa:" << name;

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_WIN)
    m_session->set_dirty();
#else
    m_session->set_dirty();
#endif

    syncWithArdour();
}

void NovaTrackListModel::removeTrack(int row)
{
    if (row < 0 || row >= static_cast<int>(m_routes.size()) || !m_session) return;

    auto routeToRemove = m_routes[static_cast<size_t>(row)];
    if (!routeToRemove) return;

    qCDebug(novaModel) << "🗑️ Eliminando pista nativa en fila:" << row << "Nombre:" << QString::fromStdString(routeToRemove->name());

    m_session->remove_route(routeToRemove);
    m_session->set_dirty();

    syncWithArdour();
}

void NovaTrackListModel::setGain(int row, float dB)
{
    if (row < 0 || row >= static_cast<int>(m_routes.size()) || !m_session) return;

    const auto &route = m_routes[static_cast<size_t>(row)];
    if (route) {
        auto gc = route->gain_control();
        if (gc) {
            double coeff = NovaAudioUtils::dbToCoeff(dB);
            gc->set_value(coeff, PBD::Controllable::NoGroup);
            m_session->set_dirty();
        }
    }
    Q_EMIT dataChanged(index(row), index(row), {GainRole});
}

void NovaTrackListModel::setMute(int row, bool muted)
{
    if (row < 0 || row >= static_cast<int>(m_routes.size()) || !m_session) return;

    const auto &route = m_routes[static_cast<size_t>(row)];
    if (route) {
        auto mc = route->mute_control();
        if (mc) {
            mc->set_value(muted ? 1.0 : 0.0, PBD::Controllable::NoGroup);
            m_session->set_dirty();
        }
    }
    Q_EMIT dataChanged(index(row), index(row), {MuteRole});
}

void NovaTrackListModel::setSolo(int row, bool soloed)
{
    if (row < 0 || row >= static_cast<int>(m_routes.size()) || !m_session) return;

    const auto &route = m_routes[static_cast<size_t>(row)];
    if (route) {
        auto sc = route->solo_control();
        if (sc) {
            sc->set_value(soloed ? 1.0 : 0.0, PBD::Controllable::NoGroup);
            m_session->set_dirty();
        }
    }
    Q_EMIT dataChanged(index(row), index(row), {SoloRole});
}

void NovaTrackListModel::setRecEnable(int row, bool enabled)
{
    if (row < 0 || row >= static_cast<int>(m_routes.size()) || !m_session) return;

    const auto &route = m_routes[static_cast<size_t>(row)];
    if (route) {
        auto track = std::dynamic_pointer_cast<ARDOUR::Track>(route);
        if (track) {
            auto rc = track->rec_enable_control();
            if (rc) {
                rc->set_value(enabled ? 1.0 : 0.0, PBD::Controllable::NoGroup);
                m_session->set_dirty();
            }
        }
    }
    Q_EMIT dataChanged(index(row), index(row), {RecEnableRole});
}

void NovaTrackListModel::setPan(int row, float pan)
{
    if (row < 0 || row >= static_cast<int>(m_routes.size()) || !m_session) return;

    const auto &route = m_routes[static_cast<size_t>(row)];
    if (route) {
        auto pc = route->pan_azimuth_control();
        if (pc) {
            // Mapear de [-1.0 (L) a +1.0 (R)] proveniente del dial QML a [0.0 a 1.0] esperado por Ardour Azimuth
            double azimuth = (pan + 1.0f) / 2.0f;
            pc->set_value(azimuth, PBD::Controllable::NoGroup);
            m_session->set_dirty();
        }
    }
    Q_EMIT dataChanged(index(row), index(row), {PanRole});
}

float NovaTrackListModel::peakLevel(int row) const
{
    if (row < 0 || row >= static_cast<int>(m_routes.size()) || !m_session) return 0.0f;
    return 0.0f;
}