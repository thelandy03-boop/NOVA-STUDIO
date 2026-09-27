#include "NovaWaveformItem.h"
#include <QDebug>
#include <vector>
#include <cmath>

#pragma push_macro("emit")
#pragma push_macro("slots")
#pragma push_macro("signals")
#pragma push_macro("foreach")
#undef emit
#undef slots
#undef signals
#undef foreach

#include "ardour/track.h"
#include "ardour/playlist.h"

#pragma pop_macro("emit")
#pragma pop_macro("slots")
#pragma pop_macro("signals")
#pragma pop_macro("foreach")

NovaWaveformItem::NovaWaveformItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAntialiasing(true); // 🎨 Nombre de método correcto en Qt6
    setFlag(ItemHasContents, true);
}

void NovaWaveformItem::setRegionIndex(int index)
{
    if (m_regionIndex == index) return;
    m_regionIndex = index;
    Q_EMIT regionIndexChanged();
    update();
}

void NovaWaveformItem::setWaveColor(const QColor &color)
{
    if (m_waveColor == color) return;
    m_waveColor = color;
    Q_EMIT waveColorChanged();
    update();
}

void NovaWaveformItem::paint(QPainter *painter)
{
    if (!s_session || m_regionIndex < 0) return;

    int widthPx = static_cast<int>(width());
    int heightPx = static_cast<int>(height());

    if (widthPx <= 0 || heightPx <= 0) return;

    // Buscar el AudioRegion correspondiente en la sesión
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

    float centerY = heightPx / 2.0f;

    // Si no se encuentra la región o no hay picos listos, dibujar onda de demostración estilizada
    if (!targetRegion) {
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(QPen(m_waveColor, 1.5));

        float amplitude = heightPx * 0.35f;
        QPainterPath path;
        path.moveTo(0, centerY);

        for (int x = 0; x < widthPx; x += 4) {
            float val = std::sin(x * 0.05f) * std::cos(x * 0.02f) * amplitude;
            path.lineTo(x, centerY - val);
        }

        painter->drawPath(path);
        return;
    }

    // Leer picos reales de la región desde Ardour Core
    size_t nPeaks = static_cast<size_t>(widthPx);
    std::vector<ARDOUR::PeakData> peaks(nPeaks);

    ARDOUR::samplecnt_t startSample = 0;
    ARDOUR::samplecnt_t lengthSamples = targetRegion->length_samples();

    ARDOUR::samplecnt_t readCount = targetRegion->read_peaks(
        peaks.data(),
        static_cast<ARDOUR::samplecnt_t>(nPeaks),
        startSample,
        lengthSamples,
        0, // canal 0
        1.0
    );

    painter->setRenderHint(QPainter::Antialiasing);

    QColor fillColor = m_waveColor;
    fillColor.setAlpha(120);

    QPen pen(m_waveColor, 1.0);
    painter->setPen(pen);
    painter->setBrush(fillColor);

    if (readCount > 0) {
        for (size_t x = 0; x < nPeaks; ++x) {
            float minVal = peaks[x].min;
            float maxVal = peaks[x].max;

            float yMax = centerY - (maxVal * centerY * 0.9f);
            float yMin = centerY - (minVal * centerY * 0.9f);

            painter->drawLine(QPointF(x, yMin), QPointF(x, yMax));
        }
    } else {
        // Onda de silueta dinámica si aún se están construyendo los picos
        for (int x = 0; x < widthPx; x += 3) {
            float h = (std::sin(x * 0.1) * 0.5f + 0.5f) * (heightPx * 0.6f);
            painter->drawLine(QPointF(x, centerY - h/2), QPointF(x, centerY + h/2));
        }
    }
}