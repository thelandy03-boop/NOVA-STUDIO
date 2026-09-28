#ifndef NOVASESSIONMANAGER_H
#define NOVASESSIONMANAGER_H

#include <QObject>
#include <QString>

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
    class AudioEngine;
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

    ARDOUR::Session* session() const { return m_session; }
    ARDOUR::AudioEngine* engine() const { return m_engine; }

signals:
    void sessionInitialized(ARDOUR::Session *session);

private:
    ARDOUR::Session *m_session = nullptr;
    ARDOUR::AudioEngine *m_engine = nullptr;
};

#endif // NOVASESSIONMANAGER_H
