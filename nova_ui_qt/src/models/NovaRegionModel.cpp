#include "NovaRegionModel.h"
#include "core/NovaLogging.h"
#include "core/NovaTimeUtils.h"
#include <QFileInfo>
#include <QDateTime>
#include <QDir>
#include <algorithm>
#include <cmath>
#include <thread>
#include <fstream>
#include <cstring>

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

// 🚀 IMPLEMENTACIÓN DE CACHÉ EN RAM
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
    m_sessionConnections.drop_connections();
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

// 🚀 PERSISTENCIA BINARIA .novapeak (16 KB por canción)
bool NovaRegionModel::savePeakFile(const QString &peakFilePath, const std::vector<PeakPoint> &peaks)
{
    std::ofstream out(peakFilePath.toStdString(), std::ios::binary);
    if (!out.is_open()) return false;

    out.write("NOVA", 4);
    uint32_t version = 1;
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

    if (numPeaks == 0 || numPeaks > 100000) return false;

    outPeaks.resize(numPeaks);
    in.read(reinterpret_cast<char*>(outPeaks.data()), numPeaks * sizeof(PeakPoint));

    return in.good();
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

    // 🚀 1. HILO SECUNDARIO: Remuestreo e Ingeniería de Picos 100% Completa
    std::thread([this, targetTrack, cleanPath, fileInfo, startBeat]() {
        try {
            ARDOUR::ImportStatus status;
            status.paths.push_back(cleanPath.toStdString());
            status.quality = ARDOUR::SrcFastest;
            status.replace_existing_source = false;
            status.split_midi_channels = false;
            status.import_markers = false;
            status.cancel = false;
            status.done = false;
            status.all_done = false;
            status.current = 0;
            status.total = 1;

            m_session->import_files(status);

            if (status.sources.empty() || status.cancel) {
                qCWarning(novaModel) << "No se pudieron generar fuentes para:" << cleanPath;
                return;
            }

            PBD::PropertyList plist;
            plist.add(ARDOUR::Properties::start, Temporal::timepos_t(0));
            plist.add(ARDOUR::Properties::length, status.sources.front()->length());
            plist.add(ARDOUR::Properties::name, fileInfo.fileName().toStdString());
            plist.add(ARDOUR::Properties::layer, 0);
            plist.add(ARDOUR::Properties::whole_file, true);
            plist.add(ARDOUR::Properties::opaque, true);

            std::shared_ptr<ARDOUR::Region> region = ARDOUR::RegionFactory::create(status.sources, plist);
            if (!region) return;

            // 🎯 2. ESCANEO SECUENCIAL 100% COMPLETO DE MUESTRAS DESDE EL AUDIO SOURCE
            const size_t numPoints = 1200;
            std::vector<PeakPoint> ramPeaks(numPoints);

            std::shared_ptr<ARDOUR::AudioSource> audioSrc = std::dynamic_pointer_cast<ARDOUR::AudioSource>(status.sources.front());
            if (audioSrc) {
                ARDOUR::samplecnt_t totalSamples = audioSrc->readable_length_samples();
                if (totalSamples > 0) {
                    ARDOUR::samplecnt_t chunkSize = totalSamples / numPoints;
                    if (chunkSize < 1) chunkSize = 1;

                    const ARDOUR::samplecnt_t BUF_SIZE = 8192;
                    std::vector<ARDOUR::Sample> buf(BUF_SIZE);

                    ARDOUR::samplepos_t currentSample = 0;

                    for (size_t p = 0; p < numPoints; ++p) {
                        ARDOUR::samplecnt_t samplesToReadForThisPoint = chunkSize;
                        float minV = 0.0f;
                        float maxV = 0.0f;
                        bool first = true;

                        while (samplesToReadForThisPoint > 0 && currentSample < totalSamples) {
                            ARDOUR::samplecnt_t toRead = std::min(samplesToReadForThisPoint, BUF_SIZE);
                            ARDOUR::samplecnt_t nRead = audioSrc->read(buf.data(), currentSample, toRead, 0);
                            if (nRead <= 0) break;

                            for (ARDOUR::samplecnt_t s = 0; s < nRead; ++s) {
                                float val = static_cast<float>(buf[s]);
                                if (first) {
                                    minV = maxV = val;
                                    first = false;
                                } else {
                                    if (val < minV) minV = val;
                                    if (val > maxV) maxV = val;
                                }
                            }
                            currentSample += nRead;
                            samplesToReadForThisPoint -= nRead;
                        }

                        ramPeaks[p].min = std::clamp(minV, -1.0f, 1.0f);
                        ramPeaks[p].max = std::clamp(maxV, -1.0f, 1.0f);
                    }
                }
            }

            QString regId = QString::fromStdString(region->id().to_s());
            NovaWaveformCache::setPeaks(regId, ramPeaks);

            // 🚀 Corregido: m_session->path() directo para obtener la carpeta de la sesión
            QString sessionPeaksDir = QString::fromStdString(m_session->path()) + "/peaks";
            QDir().mkpath(sessionPeaksDir);
            QString peakFile = sessionPeaksDir + "/" + regId + ".novapeak";
            savePeakFile(peakFile, ramPeaks);

            double sampleRate = m_session->sample_rate() > 0 ? static_cast<double>(m_session->sample_rate()) : 48000.0;
            ARDOUR::samplepos_t startSample = static_cast<ARDOUR::samplepos_t>(NovaTimeUtils::beatToFrame(startBeat, sampleRate, 120.0));

            auto playlist = targetTrack->playlist();
            if (playlist) {
                playlist->add_region(region, Temporal::timepos_t(startSample));

                // 🚀 3. HILO PRINCIPAL: Notificar actualización a QML
                QMetaObject::invokeMethod(this, [this, fileInfo]() {
                    rebuildRegionCache();
                    qCDebug(novaModel) << "✅ Audio y Caché RAM (.novapeak) procesados con éxito:" << fileInfo.fileName();
                }, Qt::QueuedConnection);
            }
        } catch (const std::exception &e) {
            qCWarning(novaModel) << "Excepción al importar audio:" << e.what();
        }
    }).detach();

    return true;
}

bool NovaRegionModel::moveRegion(int regionIndex, double newStartBeat)
{
    if (regionIndex < 0 || regionIndex >= static_cast<int>(m_regions.size())) return false;

    auto &item = m_regions[static_cast<size_t>(regionIndex)];
    double sampleRate = (m_session && m_session->sample_rate() > 0) ? static_cast<double>(m_session->sample_rate()) : 48000.0;
    ARDOUR::samplepos_t startSample = static_cast<ARDOUR::samplepos_t>(NovaTimeUtils::beatToFrame(newStartBeat, sampleRate, 120.0));

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
    double sampleRate = (m_session && m_session->sample_rate() > 0) ? static_cast<double>(m_session->sample_rate()) : 48000.0;
    
    ARDOUR::samplepos_t startSample = static_cast<ARDOUR::samplepos_t>(NovaTimeUtils::beatToFrame(newStartBeat, sampleRate, 120.0));
    ARDOUR::samplecnt_t lengthSamples = static_cast<ARDOUR::samplecnt_t>(NovaTimeUtils::beatToFrame(newLengthBeats, sampleRate, 120.0));

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

    qCDebug(novaModel) << "Region eliminada del modelo. Índice:" << regionIndex;
    Q_EMIT regionCountChanged(static_cast<int>(m_regions.size()));
}

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
    item.lengthBeats = 0.0;
    item.color = QStringLiteral("#FF3B30");
    item.isLiveRecording = true;

    m_regions.push_back(item);
    m_liveRecordingIndex = row;
    endInsertRows();

    qCDebug(novaModel) << "Clip de grabación en vivo instanciado en beat:" << startBeat;
    Q_EMIT regionCountChanged(static_cast<int>(m_regions.size()));
}

void NovaRegionModel::updateLiveRecordingClip(double lengthBeats)
{
    if (m_liveRecordingIndex < 0 || m_liveRecordingIndex >= static_cast<int>(m_regions.size())) return;

    m_regions[static_cast<size_t>(m_liveRecordingIndex)].lengthBeats = lengthBeats;
    Q_EMIT dataChanged(index(m_liveRecordingIndex), index(m_liveRecordingIndex), {LengthBeatsRole});
}

void NovaRegionModel::finalizeLiveRecordingClip()
{
    if (m_liveRecordingIndex >= 0 && m_liveRecordingIndex < static_cast<int>(m_regions.size())) {
        auto &item = m_regions[static_cast<size_t>(m_liveRecordingIndex)];
        item.isLiveRecording = false;
        item.name = QStringLiteral("Audio Take %1").arg(m_liveRecordingIndex + 1);
        item.color = QStringLiteral("#E74C3C");

        Q_EMIT dataChanged(index(m_liveRecordingIndex), index(m_liveRecordingIndex), 
                           {RegionNameRole, ColorRole, IsLiveRecordingRole});
        qCDebug(novaModel) << "Clip grabado conservado permanentemente:" << item.name << "Duración beats:" << item.lengthBeats;
    }
    m_liveRecordingIndex = -1;
}

void NovaRegionModel::rebuildRegionCache()
{
    if (!m_session) return;

    std::vector<NovaRegionItem> localPreservedClips;
    for (const auto &item : m_regions) {
        if (!item.regionPtr && item.lengthBeats > 0.1) {
            localPreservedClips.push_back(item);
        }
    }

    beginResetModel();
    m_regions.clear();

    auto routeList = m_session->get_routes();
    double sampleRate = m_session->sample_rate() > 0 ? static_cast<double>(m_session->sample_rate()) : 48000.0;
    // 🚀 Corregido: m_session->path() directo
    QString sessionPeaksDir = QString::fromStdString(m_session->path()) + "/peaks";

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

                        // 🚀 Carga ultra-rápida desde disco (.novapeak) a RAM
                        std::vector<PeakPoint> cachedPeaks;
                        if (!NovaWaveformCache::getPeaks(item.id, cachedPeaks)) {
                            QString peakFile = sessionPeaksDir + "/" + item.id + ".novapeak";
                            if (loadPeakFile(peakFile, cachedPeaks)) {
                                NovaWaveformCache::setPeaks(item.id, cachedPeaks);
                            }
                        }

                        m_regions.push_back(item);
                    }
                }
            }
            trackIdx++;
        }
    }

    for (auto &clip : localPreservedClips) {
        m_regions.push_back(clip);
    }

    endResetModel();
    qCDebug(novaModel) << "Caché de regiones reconstruido. Total regiones:" << m_regions.size();
    Q_EMIT regionCountChanged(static_cast<int>(m_regions.size()));
}