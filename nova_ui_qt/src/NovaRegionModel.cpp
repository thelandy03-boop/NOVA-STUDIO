#include "NovaRegionModel.h"
#include <QDebug>
#include <QFileInfo>
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
    case RegionIdRole:     return item.id;
    case TrackIndexRole:   return item.trackIndex;
    case RegionNameRole:   return item.name;
    case StartFrameRole:   return item.startFrame;
    case LengthFramesRole: return item.lengthFrames;
    case StartBeatRole:    return item.startBeat;
    case LengthBeatsRole:  return item.lengthBeats;
    case ColorRole:        return item.color;
    default:               return {};
    }
}

QHash<int, QByteArray> NovaRegionModel::roleNames() const
{
    return {
        { RegionIdRole,     "regionId"     },
        { TrackIndexRole,   "trackIndex"   },
        { RegionNameRole,   "regionName"   },
        { StartFrameRole,   "startFrame"   },
        { LengthFramesRole, "lengthFrames" },
        { StartBeatRole,    "startBeat"    },
        { LengthBeatsRole,  "lengthBeats"  },
        { ColorRole,        "regionColor"  }
    };
}

bool NovaRegionModel::importAudioFile(int trackIndex, const QString &filePath, double startBeat)
{
    if (!m_session) {
        qWarning() << "❌ [NovaRegion] Imposible importar audio: No hay sesión activa.";
        return false;
    }

    // Limpiar prefijo file:// si viene desde QML FileDialog
    QString cleanPath = filePath;
    if (cleanPath.startsWith("file://")) {
        cleanPath = cleanPath.mid(7);
    }

    QFileInfo fileInfo(cleanPath);
    if (!fileInfo.exists()) {
        qWarning() << "❌ [NovaRegion] El archivo de audio no existe:" << cleanPath;
        return false;
    }

    auto routeList = m_session->get_routes();
    if (!routeList || trackIndex < 0 || trackIndex >= static_cast<int>(routeList->size())) {
        qWarning() << "❌ [NovaRegion] Índice de pista inválido:" << trackIndex;
        return false;
    }

    // Buscar la pista por su índice ordenado
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

    if (!targetTrack) {
        qWarning() << "❌ [NovaRegion] La ruta seleccionada no es una pista válida.";
        return false;
    }

    qDebug() << "🎵 [NovaRegion] Importando archivo de audio:" << cleanPath << "en pista index:" << trackIndex;

    try {
        // 1. Crear el Source externo desde el archivo de audio
        std::shared_ptr<ARDOUR::Source> source = ARDOUR::SourceFactory::createExternal(
            ARDOUR::DataType::AUDIO,
            *m_session,
            cleanPath.toStdString(),
            0,
            (ARDOUR::Source::Flag)0
        );

        if (!source) {
            qWarning() << "❌ [NovaRegion] SourceFactory no pudo leer el archivo de audio.";
            return false;
        }

        // 2. Construir las propiedades requeridas por Ardour para instanciar la región
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

        // 3. Crear la región a través de RegionFactory
        std::shared_ptr<ARDOUR::Region> region = ARDOUR::RegionFactory::create(sources, plist);
        if (!region) {
            qWarning() << "❌ [NovaRegion] RegionFactory no pudo generar la región de audio.";
            return false;
        }

        // 4. Calcular la posición en muestras según el startBeat
        double bpm = 120.0;
        double sampleRate = m_session->sample_rate() > 0 ? static_cast<double>(m_session->sample_rate()) : 44100.0;
        double secondsPerBeat = 60.0 / bpm;
        ARDOUR::samplepos_t startSample = static_cast<ARDOUR::samplepos_t>(startBeat * secondsPerBeat * sampleRate);

        // 5. Insertar la región en la Playlist de la pista
        auto playlist = targetTrack->playlist();
        if (playlist) {
            playlist->add_region(region, Temporal::timepos_t(startSample));
            qDebug() << "✅ [NovaRegion] Región de audio instanciada exitosamente en la pista:" << targetTrack->name().c_str();
            
            rebuildRegionCache();
            return true;
        }
    } catch (std::exception &e) {
        qCritical() << "❌ [NovaRegion] Excepción importando audio:" << e.what();
    } catch (...) {
        qCritical() << "❌ [NovaRegion] Error desconocido al importar audio.";
    }

    return false;
}

void NovaRegionModel::removeRegion(int regionIndex)
{
    if (regionIndex < 0 || regionIndex >= static_cast<int>(m_regions.size())) return;

    beginRemoveRows(QModelIndex(), regionIndex, regionIndex);
    m_regions.erase(m_regions.begin() + regionIndex);
    endRemoveRows();

    Q_EMIT regionCountChanged(static_cast<int>(m_regions.size()));
}

void NovaRegionModel::rebuildRegionCache()
{
    if (!m_session) return;

    beginResetModel();
    m_regions.clear();

    auto routeList = m_session->get_routes();
    if (!routeList) {
        endResetModel();
        return;
    }

    double sampleRate = m_session->sample_rate() > 0 ? static_cast<double>(m_session->sample_rate()) : 44100.0;
    double bpm = 120.0;
    double secondsPerBeat = 60.0 / bpm;

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
                    
                    item.startFrame = static_cast<double>(reg->position_sample());
                    item.lengthFrames = static_cast<double>(reg->length_samples());

                    // Conversión a compases y beats musicales
                    double startSeconds = item.startFrame / sampleRate;
                    double lengthSeconds = item.lengthFrames / sampleRate;

                    item.startBeat = startSeconds / secondsPerBeat;
                    item.lengthBeats = lengthSeconds / secondsPerBeat;
                    item.color = QStringLiteral("#4A90E2");

                    m_regions.push_back(item);
                }
            }
        }
        trackIdx++;
    }

    endResetModel();
    Q_EMIT regionCountChanged(static_cast<int>(m_regions.size()));
    qDebug() << "🔄 [NovaRegion] Caché de regiones reconstruido. Total regiones:" << m_regions.size();
}