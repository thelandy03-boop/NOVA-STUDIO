#pragma once

#include <QQuickPaintedItem>
#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <memory>

// ── PROTECCIÓN CONTRA COLISIONES DE SEÑALES ARDOUR/QT ────────────────
#pragma push_macro("emit")
#pragma push_macro("slots")
#pragma push_macro("signals")
#pragma push_macro("foreach")
#undef emit
#undef slots
#undef signals
#undef foreach

#include "ardour/session.h"
#include "ardour/audioregion.h"
#include "ardour/types.h" // 🎨 Definición real de PeakData

#pragma pop_macro("emit")
#pragma pop_macro("slots")
#pragma pop_macro("signals")
#pragma pop_macro("foreach")
// ─────────────────────────────────────────────────────────────────────

class NovaWaveformItem : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(int regionIndex READ regionIndex WRITE setRegionIndex NOTIFY regionIndexChanged)
    Q_PROPERTY(QColor waveColor READ waveColor WRITE setWaveColor NOTIFY waveColorChanged)

public:
    explicit NovaWaveformItem(QQuickItem *parent = nullptr);
    ~NovaWaveformItem() override = default;

    int regionIndex() const { return m_regionIndex; }
    void setRegionIndex(int index);

    QColor waveColor() const { return m_waveColor; }
    void setWaveColor(const QColor &color);

    // QQuickPaintedItem override
    void paint(QPainter *painter) override;

    static void setSession(ARDOUR::Session *session) { s_session = session; }

signals:
    void regionIndexChanged();
    void waveColorChanged();

private:
    int m_regionIndex = -1;
    QColor m_waveColor = QColor("#A0FFFFFF");

    static inline ARDOUR::Session *s_session = nullptr;
};