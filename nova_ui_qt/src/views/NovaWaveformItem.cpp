#include "NovaWaveformItem.h"
#include "models/NovaRegionModel.h"
#include "core/NovaLogging.h"
#include "core/platform/NovaAndroidStubs.h"

#include <QPainter>
#include <QPainterPath>
#include <QColor>
#include <cmath>
#include <algorithm>
#include <vector>

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
#include "ardour/audioregion.h"
#include "ardour/session.h"
#include "ardour/playlist.h"

#pragma pop_macro("emit")
#pragma pop_macro("slots")
#pragma pop_macro("signals")
#pragma pop_macro("foreach")
#endif

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

void NovaWaveformItem::setRegionId(const QString &id)
{
    if (m_regionId != id) {
        m_regionId = id;
        Q_EMIT regionIdChanged();
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

    if (widthPx <= 0 || heightPx <= 0) return;

    std::vector<PeakPoint> ramPeaks;
    bool hasData = false;

    // 🚀 1. BÚSQUEDA DIRECTA EN RAM POR REGION_ID (Súper rápido para Grabación en Vivo y Clips)
    if (!m_regionId.isEmpty()) {
        hasData = NovaWaveformCache::getPeaks(m_regionId, ramPeaks);
    }

    // 🚀 2. FALLBACK POR ÍNDICE EN LA SESIÓN (Si no se pasó regionId explícito)
    if (!hasData && m_regionIndex >= 0 && s_session) {
        auto routeList = s_session->get_routes();
        if (routeList) {
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

            if (targetRegion) {
                QString regId = QString::fromStdString(targetRegion->id().to_s());
                hasData = NovaWaveformCache::getPeaks(regId, ramPeaks);
            }
        }
    }

    painter->setRenderHint(QPainter::Antialiasing, true);
    QColor waveFillColor = m_waveColor.isValid() ? m_waveColor : QColor(255, 255, 255);
    waveFillColor.setAlpha(255);

    if (hasData && !ramPeaks.empty()) {
        size_t ramSize = ramPeaks.size();
        float centerY = heightPx * 0.5f;
        float maxAmp = heightPx * 0.44f;

        QPolygonF polygon;
        polygon.reserve(widthPx * 2);

        // Picos superiores
        for (int x = 0; x < widthPx; ++x) {
            double normX = static_cast<double>(x) / static_cast<double>(std::max(1, widthPx - 1));
            size_t idx = static_cast<size_t>(normX * static_cast<double>(ramSize - 1));

            float maxVal = std::max(0.0f, ramPeaks[idx].max);
            float ampHeight = std::max(0.75f, maxVal * maxAmp);
            polygon << QPointF(x, centerY - ampHeight);
        }

        // Picos inferiores
        for (int x = widthPx - 1; x >= 0; --x) {
            double normX = static_cast<double>(x) / static_cast<double>(std::max(1, widthPx - 1));
            size_t idx = static_cast<size_t>(normX * static_cast<double>(ramSize - 1));

            float minVal = std::min(0.0f, ramPeaks[idx].min);
            float ampHeight = std::max(0.75f, -minVal * maxAmp);
            polygon << QPointF(x, centerY + ampHeight);
        }

        painter->setPen(Qt::NoPen);
        painter->setBrush(waveFillColor);
        painter->drawPolygon(polygon);
    } else {
        // Línea central de silencio
        float centerY = heightPx * 0.5f;
        painter->setPen(QPen(waveFillColor, 1.5));
        painter->drawLine(0, static_cast<int>(centerY), widthPx, static_cast<int>(centerY));
    }
}