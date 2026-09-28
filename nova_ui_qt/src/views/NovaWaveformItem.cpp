#include "NovaWaveformItem.h"
#include "../models/NovaRegionModel.h"
#include "../core/NovaLogging.h"
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <cmath>
#include <algorithm>
#include <vector>

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
#include "ardour/session.h"
#include "ardour/playlist.h"

#pragma pop_macro("emit")
#pragma pop_macro("slots")
#pragma pop_macro("signals")
#pragma pop_macro("foreach")

NovaWaveformItem::NovaWaveformItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setFlag(ItemHasContents, true);
}

void NovaWaveformItem::setRegionIndex(int index)
{
    if (m_regionIndex != index) {
        m_regionIndex = index;
        Q_EMIT regionIndexChanged();
        update();
    }
}

void NovaWaveformItem::setWaveColor(const QColor &color)
{
    if (m_waveColor != color) {
        m_waveColor = color;
        Q_EMIT waveColorChanged();
        update();
    }
}

void NovaWaveformItem::paint(QPainter *painter)
{
    int widthPx = static_cast<int>(width());
    int heightPx = static_cast<int>(height());

    if (widthPx <= 0 || heightPx <= 0 || !s_session) return;

    auto routeList = s_session->get_routes();
    if (!routeList) return;

    std::shared_ptr<ARDOUR::AudioRegion> targetRegion = nullptr;
    int currentIdx = 0;

    for (auto &route : *routeList) {
        if (!route || !route->is_track()) continue;
        auto track = std::dynamic_pointer_cast<ARDOUR::Track>(route);
        if (track && track->playlist()) {
            auto regList = track->playlist()->region_list();
            if (regList) {
                for (auto &reg : *regList) {
                    if (currentIdx == m_regionIndex) {
                        targetRegion = std::dynamic_pointer_cast<ARDOUR::AudioRegion>(reg);
                        break;
                    }
                    currentIdx++;
                }
            }
        }
        if (targetRegion) break;
    }

    if (!targetRegion) return;

    float centerY = heightPx / 2.0f;
    QString regId = QString::fromStdString(targetRegion->id().to_s());

    // 🚀 LECTURA DE MEMORIA RAM EN 0.0001 MS
    std::vector<PeakPoint> ramPeaks;
    bool hasData = NovaWaveformCache::getPeaks(regId, ramPeaks);

    painter->setRenderHint(QPainter::Antialiasing, true);

    if (hasData && !ramPeaks.empty()) {
        size_t ramSize = ramPeaks.size();

        QPolygonF peakPolygon;
        QPolygonF rmsPolygon;
        peakPolygon.reserve(widthPx * 2);
        rmsPolygon.reserve(widthPx * 2);

        std::vector<QPointF> peakTop, peakBottom;
        std::vector<QPointF> rmsTop, rmsBottom;
        peakTop.reserve(widthPx);
        peakBottom.reserve(widthPx);
        rmsTop.reserve(widthPx);
        rmsBottom.reserve(widthPx);

        // 🎯 MAPEO Y CÁLCULO DE DOBLE CAPA (PEAK + RMS CORE)
        for (int x = 0; x < widthPx; ++x) {
            double normX = static_cast<double>(x) / static_cast<double>(std::max(1, widthPx - 1));
            double peakIdxF = normX * static_cast<double>(ramSize - 1);
            
            size_t idx0 = static_cast<size_t>(std::floor(peakIdxF));
            size_t idx1 = std::min(idx0 + 1, ramSize - 1);
            double frac = peakIdxF - static_cast<double>(idx0);

            float minVal = ramPeaks[idx0].min * (1.0f - frac) + ramPeaks[idx1].min * frac;
            float maxVal = ramPeaks[idx0].max * (1.0f - frac) + ramPeaks[idx1].max * frac;

            // 1. Capa Exterior (Picos)
            float yMaxPeak = centerY - (maxVal * centerY * 0.90f);
            float yMinPeak = centerY - (minVal * centerY * 0.90f);

            // 2. Capa Interior (Núcleo RMS de energía)
            float rmsVal = (std::abs(maxVal) + std::abs(minVal)) * 0.5f * 0.65f;
            float yMaxRms = centerY - (rmsVal * centerY * 0.90f);
            float yMinRms = centerY + (rmsVal * centerY * 0.90f);

            if (std::abs(yMinPeak - yMaxPeak) < 1.0f) {
                yMaxPeak = centerY - 0.5f;
                yMinPeak = centerY + 0.5f;
            }

            peakTop.emplace_back(static_cast<double>(x), yMaxPeak);
            peakBottom.emplace_back(static_cast<double>(x), yMinPeak);

            rmsTop.emplace_back(static_cast<double>(x), yMaxRms);
            rmsBottom.emplace_back(static_cast<double>(x), yMinRms);
        }

        // Construir Polígonos
        for (const auto &pt : peakTop) peakPolygon << pt;
        for (auto it = peakBottom.rbegin(); it != peakBottom.rend(); ++it) peakPolygon << *it;

        for (const auto &pt : rmsTop) rmsPolygon << pt;
        for (auto it = rmsBottom.rbegin(); it != rmsBottom.rend(); ++it) rmsPolygon << *it;

        // 🎨 1. DIBUJAR CAPA EXTERIOR (PEAKS - Semi-transparente)
        QColor peakFill = m_waveColor.lighter(140);
        peakFill.setAlpha(110);
        painter->setPen(Qt::NoPen);
        painter->setBrush(peakFill);
        painter->drawPolygon(peakPolygon);

        // 🎨 2. DIBUJAR CAPA INTERIOR (RMS CORE - Sólida e Intensa estilo Ableton/Pro Tools)
        QColor rmsFill = m_waveColor.lighter(160);
        rmsFill.setAlpha(220);
        painter->setBrush(rmsFill);
        painter->drawPolygon(rmsPolygon);

        // 🎨 3. CONTORNO NÍTIDO (OUTLINE HD)
        QPen outlinePen(m_waveColor.lighter(180), 1.0);
        painter->setPen(outlinePen);

        for (size_t i = 1; i < peakTop.size(); ++i) {
            painter->drawLine(peakTop[i - 1], peakTop[i]);
            painter->drawLine(peakBottom[i - 1], peakBottom[i]);
        }
    } else {
        // Línea neutra silenciosa
        painter->setPen(QPen(m_waveColor.darker(130), 1.0));
        painter->drawLine(0, static_cast<int>(centerY), widthPx, static_cast<int>(centerY));
    }

    // Línea central de referencia
    painter->setPen(QPen(QColor(255, 255, 255, 30), 1.0, Qt::DashLine));
    painter->drawLine(0, static_cast<int>(centerY), widthPx, static_cast<int>(centerY));
}