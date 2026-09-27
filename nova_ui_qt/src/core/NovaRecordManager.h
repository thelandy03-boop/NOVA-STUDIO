#pragma once

#include <QObject>
#include <QString>

// ── PROTECCIÓN CONTRA COLISIONES ─────────────────────────────────────
#pragma push_macro("emit")
#pragma push_macro("slots")
#pragma push_macro("signals")
#pragma push_macro("foreach")
#undef emit
#undef slots
#undef signals
#undef foreach

namespace ARDOUR {
    class Session;
}

#pragma pop_macro("emit")
#pragma pop_macro("slots")
#pragma pop_macro("signals")
#pragma pop_macro("foreach")

class NovaRegionModel;
class NovaTransportController;

class NovaRecordManager : public QObject
{
    Q_OBJECT

public:
    explicit NovaRecordManager(QObject *parent = nullptr);
    ~NovaRecordManager() override = default;

    void setSession(ARDOUR::Session *session);
    void setRegionModel(NovaRegionModel *regionModel);
    void setTransportController(NovaTransportController *transport);

    void toggleRecord();
    bool isRecording() const { return m_isRecording; }

signals:
    void isRecordingChanged();

private slots:
    void onPositionChanged();
    void onTransportStopped();

private:
    ARDOUR::Session *m_session = nullptr;
    NovaRegionModel *m_regionModel = nullptr;
    NovaTransportController *m_transport = nullptr;

    bool m_isRecording = false;
    bool m_liveRegionActive = false;
    double m_recordStartBeat = 0.0;
};