#include "NovaRegionModel.h"
#include "core/NovaLogging.h"
#include "core/NovaTimeUtils.h"
#include "core/platform/NovaPlatformUtils.h"
#include "core/platform/NovaAndroidStubs.h"

#include <QFileInfo>
#include <QDateTime>
#include <QDir>
#include <QUrl>
#include <QTimer>
#include <algorithm>
#include <cmath>
#include <thread>
#include <fstream>
#include <cstring>

#if !defined(Q_OS_ANDROID)
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
#include "ardour/audiofilesource.h"
#include "ardour/audioregion.h"
#include "ardour/audiosource.h"
#include "ardour/source_factory.h"
#include "ardour/region_factory.h"
#include "ardour/data_type.h"
#include "ardour/source.h"
#include "ardour/region.h"
#include "ardour/import_status.h"

#pragma pop_macro("emit")
#pragma pop_macro("slots")
#pragma pop_macro("signals")
#pragma pop_macro("foreach")
#endif

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

static inline float coeffToDb(float coeff) {
    if (coeff <= 0.000001f) return -192.0f;
    return 20.0f * std::log10(coeff);
}

static inline float dbToCoeff(float dB) {
    if (dB <= -192.0f) return 0.0f;
    return std::pow(10.0f, dB / 20.0f);
}

static std::vector<PeakPoint> buildRegionPeaks(const std::shared_ptr<ARDOUR::AudioRegion> &region,
                                               size_t pointCount = 4000)
{
    std::vector<PeakPoint> result(pointCount, PeakPoint{0.0f, 0.0f, 0.0f});
    if (!region || pointCount == 0 || region->n_channels() == 0) return result;

    const auto length = region->length_samples();
    if (length <= 0 || length > 100000000000LL) return result;

    for (uint32_t channel = 0; channel < region->n_channels(); ++channel) {
        auto source = region->audio_source(channel);
        if (!source) continue;

        constexpr ARDOUR::samplecnt_t bufferSize = 8192;
        std::vector<ARDOUR::Sample> buffer(static_cast<size_t>(bufferSize), 0.0f);

        const ARDOUR::samplecnt_t baseSamples = length / static_cast<ARDOUR::samplecnt_t>(pointCount);
        const ARDOUR::samplecnt_t remainder = length % static_cast<ARDOUR::samplecnt_t>(pointCount);

        for (size_t i = 0; i < pointCount; ++i) {
            const auto first = static_cast<ARDOUR::samplecnt_t>(i) * baseSamples +
                               std::min<ARDOUR::samplecnt_t>(static_cast<ARDOUR::samplecnt_t>(i), remainder);
            const auto last = static_cast<ARDOUR::samplecnt_t>(i + 1) * baseSamples +
                              std::min<ARDOUR::samplecnt_t>(static_cast<ARDOUR::samplecnt_t>(i + 1), remainder);
            
            auto remaining = last - first;
            auto cursor = first;
            float low = 0.0f;
            float high = 0.0f;
            double sumSquares = 0.0;
            ARDOUR::samplecnt_t sampleCount = 0;
            bool foundSample = false;

            while (remaining > 0) {
                const auto requested = std::min(remaining, bufferSize);
                const auto read = source->read(buffer.data(), region->start_sample() + cursor, requested, 0);
                if (read <= 0) break;

                for (ARDOUR::samplecnt_t sample = 0; sample < read; ++sample) {
                    const float rawValue = static_cast<float>(buffer[static_cast<size_t>(sample)]);
                    const float value = applySoftClip(rawValue);

                    sumSquares += static_cast<double>(value * value);
                    sampleCount++;

                    if (!foundSample) {
                        low = high = value;
                        foundSample = true;
                    } else {
                        if (value < low) low = value;
                        if (value > high) high = value;
                    }
                }
                cursor += read;
                remaining -= read;
            }

            if (foundSample) {
                float calculatedRms = (sampleCount > 0) ? std::sqrt(static_cast<float>(sumSquares / sampleCount)) : 0.0f;
                if (channel == 0) {
                    result[i] = {low, high, calculatedRms};
                } else {
                    result[i].min = std::min(result[i].min, low);
                    result[i].max = std::max(result[i].max, high);
                    result[i].rms = std::max(result[i].rms, calculatedRms);
                }
            }
        }
    }
    return result;
}

void NovaWaveformCache::setPeaks(const QString &regionId, const std::vector<PeakPoint> &peaks)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    s_cache[regionId] = peaks;
}

bool NovaWaveformCache::getPeaks(const QString &regionId, std::vector<PeakPoint> &outPeaks)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    auto it = s_cache.find(regionId);
    if (it != s_cache.end() && !it->second.empty()) {
        outPeaks = it->second;
        return true;
    }
    return false;
}

void NovaWaveformCache::clear()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    s_cache.clear();
}

NovaRegionModel::NovaRegionModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

NovaRegionModel::~NovaRegionModel()
{
    waitForImports();
    m_sessionConnections.drop_connections();
}

void NovaRegionModel::waitForImports()
{
    for (auto &thread : m_importThreads) {
        if (thread.joinable()) thread.join();
    }
    m_importThreads.clear();
}

void NovaRegionModel::setSession(ARDOUR::Session *session)
{
    if (m_session == session) return;

    m_sessionConnections.drop_connections();

    beginResetModel();
    m_regions.clear();
    NovaWaveformCache::clear();
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
    case ClipGainDbRole:      return item.clipGainDb;
    case FadeInBeatsRole:     return item.fadeInBeats;
    case FadeOutBeatsRole:    return item.fadeOutBeats;
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
        { IsLiveRecordingRole, "isLiveRecording" },
        { ClipGainDbRole,      "clipGainDb"      },
        { FadeInBeatsRole,     "fadeInBeats"     },
        { FadeOutBeatsRole,    "fadeOutBeats"    }
    };
}

bool NovaRegionModel::savePeakFile(const QString &peakFilePath, const std::vector<PeakPoint> &peaks)
{
    std::ofstream out(peakFilePath.toStdString(), std::ios::binary);
    if (!out.is_open()) return false;

    out.write("NOVA", 4);
    uint32_t version = 5;
    uint32_t numPeaks = static_cast<uint32_t>(peaks.size());

    out.write(reinterpret_cast<const char*>(&version), sizeof(version));
    out.write(reinterpret_cast<const char*>(&numPeaks), sizeof(numPeaks));
    out.write(reinterpret_cast<const char*>(peaks.data()), numPeaks * sizeof(PeakPoint));
    
    out.close();
    return true;
}

bool NovaRegionModel::loadPeakFile(const QString &peakFilePath, std::vector<PeakPoint> &outPeaks)
{
    std::ifstream in(peakFilePath.toStdString(), std::ios::binary);
    if (!in.is_open()) return false;

    char magic[4];
    in.read(magic, 4);
    if (std::memcmp(magic, "NOVA", 4) != 0) return false;

    uint32_t version = 0, numPeaks = 0;
    in.read(reinterpret_cast<char*>(&version), sizeof(version));
    in.read(reinterpret_cast<char*>(&numPeaks), sizeof(numPeaks));

    if (version != 5 || numPeaks == 0 || numPeaks > 100000) return false;

    outPeaks.resize(numPeaks);
    in.read(reinterpret_cast<char*>(outPeaks.data()), numPeaks * sizeof(PeakPoint));

    return in.good();
}

bool NovaRegionModel::importAudioFile(int trackIndex, const QString &filePath, double startBeat)
{
    if (!m_session) return false;

    QString cleanPath = NovaPlatformUtils::sanitizePath(filePath);
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

    m_importThreads.emplace_back([this, targetTrack, cleanPath, fileInfo, startBeat]() {
        try {
            ARDOUR::ImportStatus status;
            status.paths.push_back(cleanPath.toStdString());
            status.quality = ARDOUR::SrcFastest;

            m_session->import_files(status);

            if (status.sources.empty() || status.cancel) return;

            const ARDOUR::SourceList importedSources = status.sources;
            QMetaObject::invokeMethod(this, [this, targetTrack, importedSources, fileInfo, startBeat]() {
                if (!m_session || importedSources.empty() || !importedSources.front()) return;

                PBD::PropertyList plist;
                plist.add(ARDOUR::Properties::start, Temporal::timepos_t(0));
                plist.add(ARDOUR::Properties::length, importedSources.front()->length());
                plist.add(ARDOUR::Properties::name, fileInfo.fileName().toStdString());

                auto region = ARDOUR::RegionFactory::create(importedSources, plist);
                if (!region) return;

                auto playlist = targetTrack->playlist();
                if (!playlist) return;

                const double sampleRate = m_session->sample_rate() > 0 ? static_cast<double>(m_session->sample_rate()) : 48000.0;
                const auto startSample = static_cast<ARDOUR::samplepos_t>(NovaTimeUtils::beatToFrame(startBeat, sampleRate, 120.0));
                
                playlist->add_region(region, Temporal::timepos_t(startSample));
                m_session->set_dirty();

                rebuildRegionCache();
            }, Qt::QueuedConnection);
        } catch (...) {}
    });

    return true;
}

bool NovaRegionModel::moveRegion(int regionIndex, double newStartBeat)
{
    if (regionIndex < 0 || regionIndex >= static_cast<int>(m_regions.size())) return false;

    auto &item = m_regions[static_cast<size_t>(regionIndex)];
    double sampleRate = (m_session && m_session->sample_rate() > 0) ? static_cast<double>(m_session->sample_rate()) : 48000.0;
    ARDOUR::samplepos_t startSample = static_cast<ARDOUR::samplepos_t>(NovaTimeUtils::beatToFrame(newStartBeat, sampleRate, 120.0));

    if (item.regionPtr && m_session) {
        item.regionPtr->set_position(Temporal::timepos_t(startSample));
        m_session->set_dirty();
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
    double sampleRate = (m_session && m_session->sample_rate() > 0) ? static_cast<double>(m_session->sample_rate()) : 48000.0;
    
    ARDOUR::samplepos_t startSample = static_cast<ARDOUR::samplepos_t>(NovaTimeUtils::beatToFrame(newStartBeat, sampleRate, 120.0));
    ARDOUR::samplecnt_t lengthSamples = static_cast<ARDOUR::samplecnt_t>(NovaTimeUtils::beatToFrame(newLengthBeats, sampleRate, 120.0));

    if (item.regionPtr && m_session) {
        item.regionPtr->set_position(Temporal::timepos_t(startSample));
        item.regionPtr->set_length(Temporal::timecnt_t(lengthSamples));
        m_session->set_dirty();
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
                            m_session->set_dirty();
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

void NovaRegionModel::setClipGainDb(int regionIndex, float dB)
{
    if (regionIndex < 0 || regionIndex >= static_cast<int>(m_regions.size())) return;
    auto &item = m_regions[static_cast<size_t>(regionIndex)];
    
    auto audioRegion = std::dynamic_pointer_cast<ARDOUR::AudioRegion>(item.regionPtr);
    if (audioRegion && m_session) {
        audioRegion->set_scale_amplitude(dbToCoeff(dB));
        m_session->set_dirty();
        item.clipGainDb = dB;
        Q_EMIT dataChanged(index(regionIndex), index(regionIndex), {ClipGainDbRole});
    }
}

void NovaRegionModel::setFadeInBeats(int regionIndex, double beats)
{
    if (regionIndex < 0 || regionIndex >= static_cast<int>(m_regions.size())) return;
    auto &item = m_regions[static_cast<size_t>(regionIndex)];

    auto audioRegion = std::dynamic_pointer_cast<ARDOUR::AudioRegion>(item.regionPtr);
    if (audioRegion && m_session) {
        double sampleRate = m_session->sample_rate() > 0 ? static_cast<double>(m_session->sample_rate()) : 48000.0;
        auto fadeSamples = static_cast<ARDOUR::samplecnt_t>(NovaTimeUtils::beatToFrame(beats, sampleRate, 120.0));

        audioRegion->set_fade_in_active(beats > 0.001);
        audioRegion->set_fade_in_length(fadeSamples);
        m_session->set_dirty();
        item.fadeInBeats = beats;

        Q_EMIT dataChanged(index(regionIndex), index(regionIndex), {FadeInBeatsRole});
    }
}

void NovaRegionModel::setFadeOutBeats(int regionIndex, double beats)
{
    if (regionIndex < 0 || regionIndex >= static_cast<int>(m_regions.size())) return;
    auto &item = m_regions[static_cast<size_t>(regionIndex)];

    auto audioRegion = std::dynamic_pointer_cast<ARDOUR::AudioRegion>(item.regionPtr);
    if (audioRegion && m_session) {
        double sampleRate = m_session->sample_rate() > 0 ? static_cast<double>(m_session->sample_rate()) : 48000.0;
        auto fadeSamples = static_cast<ARDOUR::samplecnt_t>(NovaTimeUtils::beatToFrame(beats, sampleRate, 120.0));

        audioRegion->set_fade_out_active(beats > 0.001);
        audioRegion->set_fade_out_length(fadeSamples);
        m_session->set_dirty();
        item.fadeOutBeats = beats;

        Q_EMIT dataChanged(index(regionIndex), index(regionIndex), {FadeOutBeatsRole});
    }
}

void NovaRegionModel::normalizeClip(int regionIndex)
{
    if (regionIndex < 0 || regionIndex >= static_cast<int>(m_regions.size())) return;
    auto &item = m_regions[static_cast<size_t>(regionIndex)];

    std::vector<PeakPoint> peaks;
    if (NovaWaveformCache::getPeaks(item.id, peaks) && !peaks.empty()) {
        float maxPeak = 0.0001f;
        for (const auto &p : peaks) {
            maxPeak = std::max({maxPeak, std::abs(p.min), std::abs(p.max)});
        }

        constexpr float targetHeadroom = 0.89125f; // -1.0 dBFS
        
        if (maxPeak > 0.001f) {
            float requiredGainCoeff = targetHeadroom / maxPeak;
            float targetGainDb = coeffToDb(requiredGainCoeff);
            
            targetGainDb = std::clamp(targetGainDb, -24.0f, 24.0f);
            setClipGainDb(regionIndex, targetGainDb);
        }
    }
}

void NovaRegionModel::createLiveRecordingClip(double startBeat)
{
    int armedTrackIdx = 0;
    int row = static_cast<int>(m_regions.size());
    beginInsertRows(QModelIndex(), row, row);

    NovaRegionItem item;
    item.id = QStringLiteral("rec_%1").arg(QDateTime::currentMSecsSinceEpoch());
    item.trackIndex = armedTrackIdx;
    item.name = QStringLiteral("🔴 Recording...");
    item.startBeat = startBeat;
    item.lengthBeats = 0.0;
    item.color = QStringLiteral("#FF3B30");
    item.isLiveRecording = true;
    item.clipGainDb = 0.0f;
    item.fadeInBeats = 0.0;
    item.fadeOutBeats = 0.0;

    m_regions.push_back(item);
    m_liveRecordingIndex = row;
    endInsertRows();

    Q_EMIT regionCountChanged(static_cast<int>(m_regions.size()));
}

void NovaRegionModel::updateLiveRecordingClip(double lengthBeats)
{
    if (m_liveRecordingIndex < 0 || m_liveRecordingIndex >= static_cast<int>(m_regions.size())) return;

    auto &item = m_regions[static_cast<size_t>(m_liveRecordingIndex)];
    item.lengthBeats = lengthBeats;

    Q_EMIT dataChanged(index(m_liveRecordingIndex), index(m_liveRecordingIndex), {LengthBeatsRole});
}

void NovaRegionModel::finalizeLiveRecordingClip()
{
    QTimer::singleShot(250, this, [this]() {
        m_liveRecordingIndex = -1;
        if (m_session) m_session->set_dirty();
        rebuildRegionCache();
    });
}

void NovaRegionModel::rebuildRegionCache()
{
    if (!m_session) return;

    beginResetModel();
    m_regions.clear();

    auto routeList = m_session->get_routes();
    double sampleRate = m_session->sample_rate() > 0 ? static_cast<double>(m_session->sample_rate()) : 48000.0;
    QString sessionPeaksDir = QString::fromStdString(m_session->path()) + "/peaks";
    QDir().mkpath(sessionPeaksDir);

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

                        item.startBeat = NovaTimeUtils::frameToBeat(item.startFrame, sampleRate, 120.0);
                        item.lengthBeats = NovaTimeUtils::frameToBeat(item.lengthFrames, sampleRate, 120.0);
                        item.color = QStringLiteral("#4A90E2");
                        item.isLiveRecording = false;

                        auto audioRegion = std::dynamic_pointer_cast<ARDOUR::AudioRegion>(reg);
                        if (audioRegion) {
                            item.clipGainDb = coeffToDb(audioRegion->scale_amplitude());
                        }

                        std::vector<PeakPoint> cachedPeaks;
                        if (!NovaWaveformCache::getPeaks(item.id, cachedPeaks)) {
                            QString peakFile = sessionPeaksDir + "/" + item.id + ".novapeak";
                            if (!loadPeakFile(peakFile, cachedPeaks)) {
                                if (audioRegion) {
                                    cachedPeaks = buildRegionPeaks(audioRegion);
                                    savePeakFile(peakFile, cachedPeaks);
                                }
                            }
                            if (!cachedPeaks.empty()) NovaWaveformCache::setPeaks(item.id, cachedPeaks);
                        }

                        m_regions.push_back(item);
                    }
                }
            }
            trackIdx++;
        }
    }

    endResetModel();
    Q_EMIT regionCountChanged(static_cast<int>(m_regions.size()));
}