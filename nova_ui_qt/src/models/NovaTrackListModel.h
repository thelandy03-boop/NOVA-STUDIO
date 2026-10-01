#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QByteArray>
#include <QString>
#include <QVariant>
#include <QMetaObject>
#include <QtGlobal>
#include <vector>
#include <memory>

#if defined(Q_OS_ANDROID)
#include "core/platform/NovaAndroidStubs.h"
#else
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
#include "ardour/route.h"
#include "ardour/track.h"
#include "pbd/signals.h"

#pragma pop_macro("emit")
#pragma pop_macro("slots")
#pragma pop_macro("signals")
#pragma pop_macro("foreach")
#endif
// ─────────────────────────────────────────────────────────────────────

class NovaTrackListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum TrackRoles {
        TrackIdRole      = Qt::UserRole + 1,
        TrackNameRole,
        TrackTypeRole,      // "audio" | "midi"
        GainRole,           // float (dB)
        MuteRole,           // bool
        SoloRole,           // bool
        RecEnableRole,      // bool
        PanRole,            // float (-1.0 a +1.0)
        ColorRole,          // QString color hexadecimal
        PeakRole            // float (0.0 a 1.0+, Nivel de Pico en Vivo)
    };
    Q_ENUM(TrackRoles)

    explicit NovaTrackListModel(QObject *parent = nullptr);
    ~NovaTrackListModel() override;

    void setSession(ARDOUR::Session *session);

    // QAbstractListModel overrides
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Métodos dinámicos invocables desde QML
    Q_INVOKABLE void addAudioTrack(const QString &name = QStringLiteral("Audio Track"));
    Q_INVOKABLE void addMidiTrack(const QString &name = QStringLiteral("MIDI Track"));
    Q_INVOKABLE void removeTrack(int row);
    Q_INVOKABLE void setGain(int row, float dB);
    Q_INVOKABLE void setMute(int row, bool muted);
    Q_INVOKABLE void setSolo(int row, bool soloed);
    Q_INVOKABLE void setRecEnable(int row, bool enabled);
    Q_INVOKABLE void setPan(int row, float pan);
    Q_INVOKABLE float peakLevel(int row) const; // 🎙️ Nivel de pico en vivo

signals:
    void trackCountChanged(int count);

private:
    void syncWithArdour();

    // Conversión de Amplitud Lineal de Fader <-> Decibelios (Logarítmica)
    static float coeffToDb(float coeff);
    static float dbToCoeff(float dB);

    ARDOUR::Session *m_session = nullptr;
    std::vector<std::shared_ptr<ARDOUR::Route>> m_routes;
    PBD::ScopedConnectionList m_sessionConnections;
};