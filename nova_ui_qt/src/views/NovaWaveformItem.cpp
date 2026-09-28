#include "NovaWaveformItem.h"
#include "../models/NovaRegionModel.h"
#include "../core/NovaLogging.h"
#include <QPainter>
#include <QPainterPath>
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

    // 🚀 LECTURA DIRECTA DE MEMORIA RAM EN 0.0001 MS (Cero I/O de disco)
    std::vector<PeakPoint> ramPeaks;
    bool hasData = NovaWaveformCache::getPeaks(regId, ramPeaks);

    painter->setRenderHint(QPainter::Antialiasing, true);

    QColor fillColor = m_waveColor;
    fillColor.setAlpha(195);
    QColor strokeColor = m_waveColor.lighter(135);

    painter->setPen(QPen(strokeColor, 1.0));
    painter->setBrush(fillColor);

    if (hasData && !ramPeaks.empty()) {
        QPolygonF polygon;
        size_t nPeaks = ramPeaks.size();
        polygon.reserve(nPeaks * 2);

        double xStep = static_cast<double>(widthPx) / static_cast<double>(nPeaks);

        // 🎨 Envolvente Superior
        for (size_t i = 0; i < nPeaks; ++i) {
            float maxVal = ramPeaks[i].max;
            float yMax = centerY - (maxVal * centerY * 0.88f);
            polygon << QPointF(static_cast<double>(i) * xStep, yMax);
        }

        // 🎨 Envolvente Inferior
        for (int i = static_cast<int>(nPeaks) - 1; i >= 0; --i) {
            float minVal = ramPeaks[i].min;
            float yMin = centerY - (minVal * centerY * 0.88f);
            polygon << QPointF(static_cast<double>(i) * xStep, yMin);
        }

        // 🎯 DIBUJO LLENO, DERSO Y SUAVE ESTILO REAPER / FL STUDIO
        painter->drawPolygon(polygon);
    } else {
        // Línea neutra silenciosa mientras se procesa la caché de RAM
        painter->setPen(QPen(m_waveColor.darker(130), 1.0));
        painter->drawLine(0, static_cast<int>(centerY), widthPx, static_cast<int>(centerY));
    }

    // Línea central de guía
    painter->setPen(QPen(QColor(255, 255, 255, 40), 1.0, Qt::DashLine));
    painter->drawLine(0, static_cast<int>(centerY), widthPx, static_cast<int>(centerY));
}