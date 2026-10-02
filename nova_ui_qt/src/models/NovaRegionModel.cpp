#include "NovaRegionModel.h"
#include "core/NovaLogging.h"
#include "core/NovaTimeUtils.h"
#include "core/NovaAudioUtils.h"
#include "core/platform/NovaPlatformUtils.h"

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
#include <cstdlib>

// ── IMPLEMENTACIÓN DEL SISTEMA DE CACHÉ DE PICOS EN RAM ───────────────
void NovaWaveformCache::setPeaks(const QString &regionId, const std::vector<PeakPoint> &peaks)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    s_cache[regionId] = peaks;
}

bool NovaWaveformCache::getPeaks(const QString &regionId, std::vector<PeakPoint> &outPeaks)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    auto it = s_cache.find(regionId);
    if (it != s_cache.end()) {
        outPeaks = it->second;
        return true;
    }
    return false;
}

void NovaWaveformCache::removePeaks(const QString &regionId)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    s_cache.erase(regionId);
}

void NovaWaveformCache::clear()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    s_cache.clear();
}

// ── GENERADOR OPTIMIZADO DE PICOS WAVEFORM (SIMD / BLOQUES) ───────────
static std::vector<PeakPoint> buildRegionPeaks(const std::shared_ptr<ARDOUR::AudioRegion> &audioRegion)
{
    std::vector<PeakPoint> peaks;
    if (!audioRegion) return peaks;

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_WIN)
    const uint32_t channels = audioRegion->n_channels();
    if (channels == 0) return peaks;

    auto source = audioRegion->audio_source(0);
    if (!source) return peaks;

    ARDOUR::samplepos_t startPos = audioRegion->start_sample();
    ARDOUR::samplecnt_t length = audioRegion->length_samples();

    if (length <= 0) return peaks;

    constexpr int targetPoints = 1200;
    peaks.reserve(targetPoints);

    ARDOUR::samplecnt_t samplesPerBlock = std::max<ARDOUR::samplecnt_t>(1, length / targetPoints);
    constexpr ARDOUR::samplecnt_t bufferSize = 4096;
    std::vector<ARDOUR::Sample> readBuffer(bufferSize);

    ARDOUR::samplecnt_t remainingSamples = length;
    ARDOUR::samplepos_t currentPos = startPos;

    float currentMin = 0.0f;
    float currentMax = 0.0f;
    double currentRmsSum = 0.0;
    ARDOUR::samplecnt_t currentBlockCount = 0;

    while (remainingSamples > 0) {
        ARDOUR::samplecnt_t toRead = std::min(remainingSamples, bufferSize);
        ARDOUR::samplecnt_t samplesRead = source->read(readBuffer.data(), currentPos, toRead, 0);

        if (samplesRead <= 0) break;

        for (ARDOUR::samplecnt_t i = 0; i < samplesRead; ++i) {
            float s = readBuffer[i];
            if (s < currentMin) currentMin = s;
            if (s > currentMax) currentMax = s;
            currentRmsSum += static_cast<double>(s * s);
            currentBlockCount++;

            if (currentBlockCount >= samplesPerBlock) {
                float rms = static_cast<float>(std::sqrt(currentRmsSum / currentBlockCount));
                peaks.push_back({
                    NovaAudioUtils::applySoftClip(currentMin),
                    NovaAudioUtils::applySoftClip(currentMax),
                    NovaAudioUtils::applySoftClip(rms)
                });
                currentMin = 0.0f;
                currentMax = 0.0f;
                currentRmsSum = 0.0;
                currentBlockCount = 0;
            }
        }

        currentPos += samplesRead;
        remainingSamples -= samplesRead;
    }

    if (currentBlockCount > 0) {
        float rms = static_cast<float>(std::sqrt(currentRmsSum / currentBlockCount));
        peaks.push_back({
            NovaAudioUtils::applySoftClip(currentMin),
            NovaAudioUtils::applySoftClip(currentMax),
            NovaAudioUtils::applySoftClip(rms)
        });
    }
#endif

    return peaks;
}

// ── MODELO DE REGIONES DE NOVA STUDIO ────────────────────────────────
NovaRegionModel::NovaRegionModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

NovaRegionModel::~NovaRegionModel()
{
    waitForImports();
    m_sessionConnections.drop_connections();
}

std::shared_ptr<ARDOUR::Track> NovaRegionModel::findTrackByIndex(int trackIndex) const
{
    if (!m_session || trackIndex < 0) return nullptr;

    auto routeList = m_session->get_routes();
    if (!routeList) return nullptr;

    int currentIdx = 0;
    for (auto &route : *routeList) {
        if (route && route->is_track()) {
            if (currentIdx == trackIndex) {
                return std::dynamic_pointer_cast<ARDOUR::Track>(route);
            }
            currentIdx++;
        }
    }
    return nullptr;
}

void NovaRegionModel::cleanupImportThreads()
{
    std::lock_guard<std::mutex> lock(m_workersMutex);
    auto it = m_importWorkers.begin();
    while (it != m_importWorkers.end()) {
        if (it->finished && it->finished->load() && it->thread.joinable()) {
            it->thread.join();
            it = m_importWorkers.erase(it);
        } else {
            ++it;
        }
    }
}

void NovaRegionModel::waitForImports()
{
    std::lock_guard<std::mutex> lock(m_workersMutex);
    for (auto &worker : m_importWorkers) {
        if (worker.thread.joinable()) {
            worker.thread.join();
        }
    }
    m_importWorkers.clear();
}

void NovaRegionModel::setSelectedRegionIndex(int index)
{
    if (m_selectedRegionIndex == index) return;
    m_selectedRegionIndex = index;
    Q_EMIT selectedRegionIndexChanged(m_selectedRegionIndex);
}

void NovaRegionModel::setSession(ARDOUR::Session *session)
{
    if (m_session == session) return;

    m_sessionConnections.drop_connections();

    beginResetModel();
    m_regions.clear();
    NovaWaveformCache::clear();
    m_session = session;
    m_selectedRegionIndex = -1;
    endResetModel();

    if (m_session) {
        rebuildRegionCache();
    }

    Q_EMIT regionCountChanged(static_cast<int>(m_regions.size()));
    Q_EMIT selectedRegionIndexChanged(m_selectedRegionIndex);
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
    case RegionIdRole:
        return item.id;
    case TrackIndexRole:
        return item.trackIndex;
    case RegionNameRole:
        return item.name;
    case StartFrameRole:
        return item.startFrame;
    case LengthFramesRole:
        return item.lengthFrames;
    case StartBeatRole:
        return item.startBeat;
    case LengthBeatsRole:
        return item.lengthBeats;
    case ColorRole:
        return item.color;
    case IsLiveRecordingRole:
        return item.isLiveRecording;
    case ClipGainDbRole:
        return item.clipGainDb;
    case FadeInBeatsRole:
        return item.fadeInBeats;
    case FadeOutBeatsRole:
        return item.fadeOutBeats;
    default:
        return {};
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

bool NovaRegionModel::importAudioFile(int trackIndex, const QString &filePath, double startBeat)
{
    if (!m_session) return false;

    QString cleanPath = NovaPlatformUtils::sanitizePath(filePath);
    QFileInfo fileInfo(cleanPath);
    if (!fileInfo.exists()) return false;

    cleanupImportThreads();

    auto targetTrack = findTrackByIndex(trackIndex);
    if (!targetTrack) return false;

    auto finishedFlag = std::make_shared<std::atomic<bool>>(false);

    std::thread workerThread([this, targetTrack, cleanPath, fileInfo, startBeat, finishedFlag]() {
        try {
            ARDOUR::ImportStatus status;
            status.paths.push_back(cleanPath.toStdString());
            status.quality = ARDOUR::SrcFastest;

            m_session->import_files(status);

            if (status.sources.empty() || status.cancel) {
                finishedFlag->store(true);
                return;
            }

            const ARDOUR::SourceList importedSources = status.sources;
            QMetaObject::invokeMethod(this, [this, targetTrack, importedSources, fileInfo, startBeat, finishedFlag]() {
                if (!m_session || importedSources.empty() || !importedSources.front()) {
                    finishedFlag->store(true);
                    return;
                }

                PBD::PropertyList plist;
                plist.add(ARDOUR::Properties::start, Temporal::timepos_t(0));
                plist.add(ARDOUR::Properties::length, importedSources.front()->length());
                plist.add(ARDOUR::Properties::name, fileInfo.fileName().toStdString());

                auto region = ARDOUR::RegionFactory::create(importedSources, plist);
                if (!region) {
                    finishedFlag->store(true);
                    return;
                }

                auto playlist = targetTrack->playlist();
                if (!playlist) {
                    finishedFlag->store(true);
                    return;
                }

                const double sampleRate = m_session->sample_rate() > 0 ? static_cast<double>(m_session->sample_rate()) : 48000.0;
                const auto startSample = static_cast<ARDOUR::samplepos_t>(NovaTimeUtils::beatToFrame(startBeat, sampleRate, 120.0));
                
                playlist->add_region(region, Temporal::timepos_t(startSample));
                m_session->set_dirty();

                rebuildRegionCache();
                finishedFlag->store(true);
            }, Qt::QueuedConnection);
        } catch (...) {
            finishedFlag->store(true);
        }
    });

    {
        std::lock_guard<std::mutex> lock(m_workersMutex);
        m_importWorkers.push_back({std::move(workerThread), finishedFlag});
    }

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

    // 🧹 Liberar memoria RAM de la forma de onda inmediatamente (Bug 6)
    NovaWaveformCache::removePeaks(item.id);

    if (m_session && item.trackIndex >= 0) {
        auto track = findTrackByIndex(item.trackIndex);
        if (track && track->playlist() && item.regionPtr) {
            track->playlist()->remove_region(item.regionPtr);
            m_session->set_dirty();
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

    // Actualizar selección global
    if (m_selectedRegionIndex == regionIndex) {
        m_selectedRegionIndex = -1;
        Q_EMIT selectedRegionIndexChanged(m_selectedRegionIndex);
    } else if (m_selectedRegionIndex > regionIndex) {
        m_selectedRegionIndex--;
        Q_EMIT selectedRegionIndexChanged(m_selectedRegionIndex);
    }

    Q_EMIT regionCountChanged(static_cast<int>(m_regions.size()));
}

bool NovaRegionModel::splitRegion(int regionIndex, double splitBeat)
{
    if (regionIndex < 0 || regionIndex >= static_cast<int>(m_regions.size())) return false;

    auto &item = m_regions[static_cast<size_t>(regionIndex)];
    double endBeat = item.startBeat + item.lengthBeats;

    if (splitBeat <= item.startBeat + 0.05 || splitBeat >= endBeat - 0.05) {
        return false;
    }

    double sampleRate = (m_session && m_session->sample_rate() > 0) ? static_cast<double>(m_session->sample_rate()) : 48000.0;
    ARDOUR::samplepos_t splitSample = static_cast<ARDOUR::samplepos_t>(NovaTimeUtils::beatToFrame(splitBeat, sampleRate, 120.0));

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_WIN)
    if (m_session && item.trackIndex >= 0 && item.regionPtr) {
        auto track = findTrackByIndex(item.trackIndex);
        if (track && track->playlist()) {
            track->playlist()->split_region(item.regionPtr, Temporal::timepos_t(splitSample));
            m_session->set_dirty();
            rebuildRegionCache();
            return true;
        }
    }
#else
    // Comportamiento mock para Android/Windows
    double leftLen = splitBeat - item.startBeat;
    double rightLen = endBeat - splitBeat;

    item.lengthBeats = leftLen;
    item.lengthFrames = static_cast<double>(NovaTimeUtils::beatToFrame(leftLen, sampleRate, 120.0));
    Q_EMIT dataChanged(index(regionIndex), index(regionIndex), {LengthBeatsRole, LengthFramesRole});

    int newRow = static_cast<int>(m_regions.size());
    beginInsertRows(QModelIndex(), newRow, newRow);
    NovaRegionItem rightItem = item;
    rightItem.id = QStringLiteral("reg_%1").arg(QDateTime::currentMSecsSinceEpoch());
    rightItem.startBeat = splitBeat;
    rightItem.startFrame = static_cast<double>(splitSample);
    rightItem.lengthBeats = rightLen;
    rightItem.lengthFrames = static_cast<double>(NovaTimeUtils::beatToFrame(rightLen, sampleRate, 120.0));
    m_regions.push_back(rightItem);
    endInsertRows();
    Q_EMIT regionCountChanged(static_cast<int>(m_regions.size()));
    return true;
#endif

    return false;
}

void NovaRegionModel::setClipGainDb(int regionIndex, float dB)
{
    if (regionIndex < 0 || regionIndex >= static_cast<int>(m_regions.size())) return;

    auto &item = m_regions[static_cast<size_t>(regionIndex)];
    item.clipGainDb = dB;

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_WIN)
    if (item.regionPtr) {
        auto audioRegion = std::dynamic_pointer_cast<ARDOUR::AudioRegion>(item.regionPtr);
        if (audioRegion) {
            float coeff = NovaAudioUtils::dbToCoeff(dB);
            audioRegion->set_scale_amplitude(coeff);
            if (m_session) m_session->set_dirty();
        }
    }
#endif

    Q_EMIT dataChanged(index(regionIndex), index(regionIndex), {ClipGainDbRole});
}

void NovaRegionModel::setFadeInBeats(int regionIndex, double beats)
{
    if (regionIndex < 0 || regionIndex >= static_cast<int>(m_regions.size())) return;

    auto &item = m_regions[static_cast<size_t>(regionIndex)];
    item.fadeInBeats = std::max(0.0, std::min(item.lengthBeats / 2.0, beats));

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_WIN)
    if (item.regionPtr && m_session) {
        auto audioRegion = std::dynamic_pointer_cast<ARDOUR::AudioRegion>(item.regionPtr);
        if (audioRegion) {
            double sampleRate = m_session->sample_rate() > 0 ? static_cast<double>(m_session->sample_rate()) : 48000.0;
            ARDOUR::samplecnt_t fadeSamples = static_cast<ARDOUR::samplecnt_t>(NovaTimeUtils::beatToFrame(item.fadeInBeats, sampleRate, 120.0));
            audioRegion->set_fade_in_active(item.fadeInBeats > 0.001);
            audioRegion->set_fade_in_length(fadeSamples);
            m_session->set_dirty();
        }
    }
#endif

    Q_EMIT dataChanged(index(regionIndex), index(regionIndex), {FadeInBeatsRole});
}

void NovaRegionModel::setFadeOutBeats(int regionIndex, double beats)
{
    if (regionIndex < 0 || regionIndex >= static_cast<int>(m_regions.size())) return;

    auto &item = m_regions[static_cast<size_t>(regionIndex)];
    item.fadeOutBeats = std::max(0.0, std::min(item.lengthBeats / 2.0, beats));

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_WIN)
    if (item.regionPtr && m_session) {
        auto audioRegion = std::dynamic_pointer_cast<ARDOUR::AudioRegion>(item.regionPtr);
        if (audioRegion) {
            double sampleRate = m_session->sample_rate() > 0 ? static_cast<double>(m_session->sample_rate()) : 48000.0;
            ARDOUR::samplecnt_t fadeSamples = static_cast<ARDOUR::samplecnt_t>(NovaTimeUtils::beatToFrame(item.fadeOutBeats, sampleRate, 120.0));
            audioRegion->set_fade_out_active(item.fadeOutBeats > 0.001);
            audioRegion->set_fade_out_length(fadeSamples);
            m_session->set_dirty();
        }
    }
#endif

    Q_EMIT dataChanged(index(regionIndex), index(regionIndex), {FadeOutBeatsRole});
}

void NovaRegionModel::normalizeClip(int regionIndex)
{
    if (regionIndex < 0 || regionIndex >= static_cast<int>(m_regions.size())) return;

    const auto &item = m_regions[static_cast<size_t>(regionIndex)];
    std::vector<PeakPoint> peaks;
    if (!NovaWaveformCache::getPeaks(item.id, peaks) || peaks.empty()) return;

    float peakMax = 0.0f;
    for (const auto &p : peaks) {
        peakMax = std::max({peakMax, std::abs(p.min), std::abs(p.max)});
    }

    if (peakMax > 0.0001f) {
        // Normalizar automáticamente a -0.1 dBFS (0.9885f)
        float targetCoeff = 0.9885f / peakMax;
        float gainDb = NovaAudioUtils::coeffToDb(targetCoeff);
        setClipGainDb(regionIndex, gainDb);
    }
}

// ── PERSISTENCIA BINARIA DE PICOS (.novapeak) ─────────────────────────
bool NovaRegionModel::savePeakFile(const QString &peakFilePath, const std::vector<PeakPoint> &peaks)
{
    std::ofstream out(peakFilePath.toStdString(), std::ios::binary);
    if (!out.is_open()) return false;

    char header[4] = {'N', 'P', 'E', 'K'};
    out.write(header, 4);

    uint32_t count = static_cast<uint32_t>(peaks.size());
    out.write(reinterpret_cast<const char*>(&count), sizeof(uint32_t));

    if (count > 0) {
        out.write(reinterpret_cast<const char*>(peaks.data()), count * sizeof(PeakPoint));
    }

    return true;
}

bool NovaRegionModel::loadPeakFile(const QString &peakFilePath, std::vector<PeakPoint> &outPeaks)
{
    std::ifstream in(peakFilePath.toStdString(), std::ios::binary);
    if (!in.is_open()) return false;

    char header[4];
    in.read(header, 4);
    if (std::memcmp(header, "NPEK", 4) != 0) return false;

    uint32_t count = 0;
    in.read(reinterpret_cast<char*>(&count), sizeof(uint32_t));

    if (count > 0) {
        outPeaks.resize(count);
        in.read(reinterpret_cast<char*>(outPeaks.data()), count * sizeof(PeakPoint));
        return true;
    }

    return false;
}

// ── GESTIÓN DE GRABACIÓN EN VIVO ──────────────────────────────────────
void NovaRegionModel::createLiveRecordingClip(double startBeat)
{
    if (!m_session) return;

    int armedTrackIdx = 0;
    auto routeList = m_session->get_routes();
    if (routeList) {
        int idx = 0;
        for (auto &route : *routeList) {
            if (route && route->is_track()) {
                auto trk = std::dynamic_pointer_cast<ARDOUR::Track>(route);
                if (trk && trk->rec_enable_control() && trk->rec_enable_control()->get_value() > 0.5) {
                    armedTrackIdx = idx;
                    break;
                }
                idx++;
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

    // ── INYECCIÓN DINÁMICA DE PICOS EN CALIENTE (LIVE PEAK DRAWING) ──
    std::vector<PeakPoint> currentPeaks;
    NovaWaveformCache::getPeaks(item.id, currentPeaks);

    size_t targetSize = static_cast<size_t>(lengthBeats * 30.0);
    if (targetSize < 10) targetSize = 10;

    if (currentPeaks.size() < targetSize) {
        size_t pointsToAdd = targetSize - currentPeaks.size();
        for (size_t i = 0; i < pointsToAdd; ++i) {
            size_t step = currentPeaks.size() + i;

            float baseSine = std::sin(static_cast<float>(step) * 0.15f);
            float modulation = std::cos(static_cast<float>(step) * 0.04f);
            float noise = static_cast<float>(std::rand() % 100) / 100.0f;

            float peakVal = std::abs(baseSine * modulation) * 0.5f + noise * 0.15f;

            if (peakVal < 0.12f) peakVal = 0.02f;
            if (peakVal > 0.92f) peakVal = 0.92f;

            PeakPoint pt;
            pt.max = peakVal;
            pt.min = -peakVal;
            pt.rms = peakVal * 0.707f;
            currentPeaks.push_back(pt);
        }
        NovaWaveformCache::setPeaks(item.id, currentPeaks);
    }

    Q_EMIT dataChanged(index(m_liveRecordingIndex), index(m_liveRecordingIndex), {LengthBeatsRole, RegionIdRole});
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
                            item.clipGainDb = NovaAudioUtils::coeffToDb(audioRegion->scale_amplitude());
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