#include "NovaRegionModel.h"
#include <QDebug>
#include <QFileInfo>
#include <QDateTime>
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
#include "ardour/source_factory.h"
#include "ardour/region_factory.h"
#include "ardour/data_type.h"
#include "ardour/source.h"
#include "ardour/region.h"

#pragma pop_macro("emit")
#pragma pop_macro("slots")
#pragma pop_macro("signals")
#pragma pop_macro("foreach")

NovaRegionModel::NovaRegionModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

NovaRegionModel::~NovaRegionModel()
{
    m_sessionConnections.drop_connections();
}

void NovaRegionModel::setSession(ARDOUR::Session *session)
{
    if (m_session == session) return;

    m_sessionConnections.drop_connections();

    beginResetModel();
    m_regions.clear();
    m_session = session;
    endResetModel();

    if (m_session) {
        rebuildRegionCache();
    }

    Q_EMIT regionCountChanged(static_cast<int>(m_regions.size()));
}

int NovaRegionModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(m_regions.size());
}

QVariant NovaRegionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_regions.size()))
        return {};

    const auto &item = m_regions[static_cast<size_t>(index.row())];

    switch (role) {
    case RegionIdRole:        return item.id;
    case TrackIndexRole:      return item.trackIndex;
    case RegionNameRole:      return item.name;
    case StartFrameRole:      return item.startFrame;
    case LengthFramesRole:    return item.lengthFrames;
    case StartBeatRole:       return item.startBeat;
    case LengthBeatsRole:     return item.lengthBeats;
    case ColorRole:           return item.color;
    case IsLiveRecordingRole: return item.isLiveRecording;
    default:                  return {};
    }
}

QHash<int, QByteArray> NovaRegionModel::roleNames() const
{
    return {
        { RegionIdRole,        "regionId"        },
        { TrackIndexRole,      "trackIndex"      },
        { RegionNameRole,      "regionName"      },
        { StartFrameRole,      "startFrame"      },
        { LengthFramesRole,    "lengthFrames"    },
        { StartBeatRole,       "startBeat"       },
        { LengthBeatsRole,     "lengthBeats"     },
        { ColorRole,           "regionColor"     },
        { IsLiveRecordingRole, "isLiveRecording" }
    };
}

bool NovaRegionModel::importAudioFile(int trackIndex, const QString &filePath, double startBeat)
{
    if (!m_session) return false;

    QString cleanPath = filePath;
    if (cleanPath.startsWith("file://")) {
        cleanPath = cleanPath.mid(7);
    }

    QFileInfo fileInfo(cleanPath);
    if (!fileInfo.exists()) return false;

    auto routeList = m_session->get_routes();
    if (!routeList || trackIndex < 0 || trackIndex >= static_cast<int>(routeList->size())) return false;

    int currentIdx = 0;
    std::shared_ptr<ARDOUR::Track> targetTrack = nullptr;

    for (auto &route : *routeList) {
        if (route && route->is_track()) {
            if (currentIdx == trackIndex) {
                targetTrack = std::dynamic_pointer_cast<ARDOUR::Track>(route);
                break;
            }
            currentIdx++;
        }
    }

    if (!targetTrack) return false;

    try {
        std::shared_ptr<ARDOUR::Source> source = ARDOUR::SourceFactory::createExternal(
            ARDOUR::DataType::AUDIO,
            *m_session,
            cleanPath.toStdString(),
            0,
            (ARDOUR::Source::Flag)0
        );

        if (!source) return false;

        PBD::PropertyList plist;
        plist.add(ARDOUR::Properties::start, Temporal::timepos_t(0));
        plist.add(ARDOUR::Properties::length, source->length());
        plist.add(ARDOUR::Properties::name, fileInfo.fileName().toStdString());
        plist.add(ARDOUR::Properties::layer, 0);
        plist.add(ARDOUR::Properties::whole_file, true);
        plist.add(ARDOUR::Properties::external, true);
        plist.add(ARDOUR::Properties::opaque, true);

        ARDOUR::SourceList sources;
        sources.push_back(source);

        std::shared_ptr<ARDOUR::Region> region = ARDOUR::RegionFactory::create(sources, plist);
        if (!region) return false;

        double bpm = 120.0;
        double sampleRate = m_session->sample_rate() > 0 ? static_cast<double>(m_session->sample_rate()) : 44100.0;
        double secondsPerBeat = 60.0 / bpm;
        ARDOUR::samplepos_t startSample = static_cast<ARDOUR::samplepos_t>(startBeat * secondsPerBeat * sampleRate);

        auto playlist = targetTrack->playlist();
        if (playlist) {
            playlist->add_region(region, Temporal::timepos_t(startSample));
            rebuildRegionCache();
            return true;
        }
    } catch (...) {}

    return false;
}

bool NovaRegionModel::moveRegion(int regionIndex, double newStartBeat)
{
    if (regionIndex < 0 || regionIndex >= static_cast<int>(m_regions.size())) return false;

    auto &item = m_regions[static_cast<size_t>(regionIndex)];
    double bpm = 120.0;
    double sampleRate = (m_session && m_session->sample_rate() > 0) ? static_cast<double>(m_session->sample_rate()) : 44100.0;
    double secondsPerBeat = 60.0 / bpm;
    ARDOUR::samplepos_t startSample = static_cast<ARDOUR::samplepos_t>(newStartBeat * secondsPerBeat * sampleRate);

    if (item.regionPtr) {
        item.regionPtr->set_position(Temporal::timepos_t(startSample));
    }

    item.startBeat = newStartBeat;
    item.startFrame = static_cast<double>(startSample);

    Q_EMIT dataChanged(index(regionIndex), index(regionIndex), {StartBeatRole, StartFrameRole});
    return true;
}

bool NovaRegionModel::resizeRegion(int regionIndex, double newStartBeat, double newLengthBeats)
{
    if (regionIndex < 0 || regionIndex >= static_cast<int>(m_regions.size())) return false;

    auto &item = m_regions[static_cast<size_t>(regionIndex)];
    double bpm = 120.0;
    double sampleRate = (m_session && m_session->sample_rate() > 0) ? static_cast<double>(m_session->sample_rate()) : 44100.0;
    double secondsPerBeat = 60.0 / bpm;
    ARDOUR::samplepos_t startSample = static_cast<ARDOUR::samplepos_t>(newStartBeat * secondsPerBeat * sampleRate);
    ARDOUR::samplecnt_t lengthSamples = static_cast<ARDOUR::samplecnt_t>(newLengthBeats * secondsPerBeat * sampleRate);

    if (item.regionPtr) {
        item.regionPtr->set_position(Temporal::timepos_t(startSample));
        item.regionPtr->set_length(Temporal::timecnt_t(lengthSamples));
    }

    item.startBeat = newStartBeat;
    item.lengthBeats = newLengthBeats;
    item.startFrame = static_cast<double>(startSample);
    item.lengthFrames = static_cast<double>(lengthSamples);

    Q_EMIT dataChanged(index(regionIndex), index(regionIndex), {StartBeatRole, LengthBeatsRole, StartFrameRole, LengthFramesRole});
    return true;
}

void NovaRegionModel::removeRegion(int regionIndex)
{
    if (regionIndex < 0 || regionIndex >= static_cast<int>(m_regions.size())) return;

    auto item = m_regions[static_cast<size_t>(regionIndex)];

    if (m_session && item.trackIndex >= 0) {
        auto routeList = m_session->get_routes();
        if (routeList) {
            int currentIdx = 0;
            for (auto &route : *routeList) {
                if (route && route->is_track()) {
                    if (currentIdx == item.trackIndex) {
                        auto track = std::dynamic_pointer_cast<ARDOUR::Track>(route);
                        if (track && track->playlist() && item.regionPtr) {
                            track->playlist()->remove_region(item.regionPtr);
                        }
                        break;
                    }
                    currentIdx++;
                }
            }
        }
    }

    beginRemoveRows(QModelIndex(), regionIndex, regionIndex);
    m_regions.erase(m_regions.begin() + regionIndex);
    endRemoveRows();

    if (m_liveRecordingIndex == regionIndex) {
        m_liveRecordingIndex = -1;
    } else if (m_liveRecordingIndex > regionIndex) {
        m_liveRecordingIndex--;
    }

    Q_EMIT regionCountChanged(static_cast<int>(m_regions.size()));
}


// 🎙️ CREAR CLIP EN VIVO
void NovaRegionModel::createLiveRecordingClip(double startBeat)
{
    int armedTrackIdx = 0;
    if (m_session) {
        auto routeList = m_session->get_routes();
        if (routeList) {
            int idx = 0;
            for (auto &route : *routeList) {
                if (route && route->is_track()) {
                    auto track = std::dynamic_pointer_cast<ARDOUR::Track>(route);
                    if (track) {
                        auto rec = track->rec_enable_control();
                        if (rec && rec->get_value() != 0.0f) {
                            armedTrackIdx = idx;
                            break;
                        }
                    }
                    idx++;
                }
            }
        }
    }

    int row = static_cast<int>(m_regions.size());
    beginInsertRows(QModelIndex(), row, row);

    NovaRegionItem item;
    item.id = QStringLiteral("rec_%1").arg(QDateTime::currentMSecsSinceEpoch());
    item.trackIndex = armedTrackIdx;
    item.name = QStringLiteral("🔴 Recording...");
    item.startBeat = startBeat;
    item.lengthBeats = 0.0; // 🔒 Arranca exactamente en 0.0
    item.color = QStringLiteral("#FF3B30");
    item.isLiveRecording = true;

    m_regions.push_back(item);
    m_liveRecordingIndex = row;
    endInsertRows();

    Q_EMIT regionCountChanged(static_cast<int>(m_regions.size()));
}

void NovaRegionModel::updateLiveRecordingClip(double lengthBeats)
{
    if (m_liveRecordingIndex < 0 || m_liveRecordingIndex >= static_cast<int>(m_regions.size())) return;

    m_regions[static_cast<size_t>(m_liveRecordingIndex)].lengthBeats = lengthBeats;
    Q_EMIT dataChanged(index(m_liveRecordingIndex), index(m_liveRecordingIndex), {LengthBeatsRole});
}

// 🎙️ FINALIZAR GRABACIÓN Y CONSERVAR PERMANENTEMENTE EL CLIP EN PANTALLA
void NovaRegionModel::finalizeLiveRecordingClip()
{
    if (m_liveRecordingIndex >= 0 && m_liveRecordingIndex < static_cast<int>(m_regions.size())) {
        auto &item = m_regions[static_cast<size_t>(m_liveRecordingIndex)];
        item.isLiveRecording = false;
        item.name = QStringLiteral("Audio Take %1").arg(m_liveRecordingIndex + 1);
        item.color = QStringLiteral("#E74C3C"); // Rojo definitivo grabado

        Q_EMIT dataChanged(index(m_liveRecordingIndex), index(m_liveRecordingIndex), 
                           {RegionNameRole, ColorRole, IsLiveRecordingRole});
        qDebug() << "✅ [NovaRegion] Clip grabado conservado permanentemente:" << item.name << "Duración beats:" << item.lengthBeats;
    }
    m_liveRecordingIndex = -1;
}

void NovaRegionModel::rebuildRegionCache()
{
    if (!m_session) return;

    // Guardar clips grabados que el backend Dummy no escribió en disco
    std::vector<NovaRegionItem> localPreservedClips;
    for (const auto &item : m_regions) {
        if (!item.regionPtr && item.lengthBeats > 0.1) {
            localPreservedClips.push_back(item);
        }
    }

    beginResetModel();
    m_regions.clear();

    auto routeList = m_session->get_routes();
    double sampleRate = m_session->sample_rate() > 0 ? static_cast<double>(m_session->sample_rate()) : 44100.0;
    double bpm = 120.0;
    double secondsPerBeat = 60.0 / bpm;

    if (routeList) {
        int trackIdx = 0;
        for (auto &route : *routeList) {
            if (!route || !route->is_track()) continue;

            auto track = std::dynamic_pointer_cast<ARDOUR::Track>(route);
            if (track && track->playlist()) {
                auto regList = track->playlist()->region_list();
                if (regList) {
                    for (auto &reg : *regList) {
                        if (!reg) continue;

                        NovaRegionItem item;
                        item.id = QString::fromStdString(reg->id().to_s());
                        item.trackIndex = trackIdx;
                        item.name = QString::fromStdString(reg->name());
                        item.regionPtr = reg;
                        
                        item.startFrame = static_cast<double>(reg->position_sample());
                        item.lengthFrames = static_cast<double>(reg->length_samples());

                        double startSeconds = item.startFrame / sampleRate;
                        double lengthSeconds = item.lengthFrames / sampleRate;

                        item.startBeat = startSeconds / secondsPerBeat;
                        item.lengthBeats = lengthSeconds / secondsPerBeat;
                        item.color = QStringLiteral("#4A90E2");
                        item.isLiveRecording = false;

                        m_regions.push_back(item);
                    }
                }
            }
            trackIdx++;
        }
    }

    // Reincorporar clips preservados
    for (auto &clip : localPreservedClips) {
        m_regions.push_back(clip);
    }

    endResetModel();
    Q_EMIT regionCountChanged(static_cast<int>(m_regions.size()));
}