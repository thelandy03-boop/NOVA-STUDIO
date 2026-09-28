#ifndef NOVA_WAVEFORM_ITEM_H
#define NOVA_WAVEFORM_ITEM_H

#include <QQuickPaintedItem>
#include <QColor>
#include <memory>

namespace ARDOUR {
    class Session;
}

class NovaWaveformItem : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(int regionIndex READ regionIndex WRITE setRegionIndex NOTIFY regionIndexChanged)
    Q_PROPERTY(QString regionId READ regionId WRITE setRegionId NOTIFY regionIdChanged)
    Q_PROPERTY(QColor waveColor READ waveColor WRITE setWaveColor NOTIFY waveColorChanged)

public:
    explicit NovaWaveformItem(QQuickItem *parent = nullptr);

    int regionIndex() const { return m_regionIndex; }
    void setRegionIndex(int index);

    QString regionId() const { return m_regionId; }
    void setRegionId(const QString &id);

    QColor waveColor() const { return m_waveColor; }
    void setWaveColor(const QColor &color);

    static void setSession(ARDOUR::Session *session) { s_session = session; }

    void paint(QPainter *painter) override;

Q_SIGNALS:
    void regionIndexChanged();
    void regionIdChanged();
    void waveColorChanged();

private:
    int m_regionIndex = -1;
    QString m_regionId;
    QColor m_waveColor = QColor(255, 255, 255);
    static inline ARDOUR::Session *s_session = nullptr;
};

#endif // NOVA_WAVEFORM_ITEM_H