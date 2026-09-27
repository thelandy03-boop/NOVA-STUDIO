#pragma once

#include <QObject>
#include <QString>
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

namespace ARDOUR {
    class AudioEngine;
    class Session;
}

#pragma pop_macro("emit")
#pragma pop_macro("slots")
#pragma pop_macro("signals")
#pragma pop_macro("foreach")

class NovaSessionManager : public QObject
{
    Q_OBJECT

public:
    explicit NovaSessionManager(QObject *parent = nullptr);
    ~NovaSessionManager() override;

    bool initSession();

    ARDOUR::AudioEngine* engine() const { return m_engine; }
    ARDOUR::Session* session() const { return m_session; }

signals:
    void sessionInitialized(ARDOUR::Session *session);

private:
    ARDOUR::AudioEngine *m_engine = nullptr;
    ARDOUR::Session *m_session = nullptr;
};