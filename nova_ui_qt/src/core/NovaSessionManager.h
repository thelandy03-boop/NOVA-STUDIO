#ifndef NOVASESSIONMANAGER_H
#define NOVASESSIONMANAGER_H

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

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

    bool initSession(); // Inicializa el motor por defecto

    // 💾 Control de Proyectos
    bool saveSession();
    bool isDirty() const;
    bool loadSession(const QString &sessionPath);
    bool createNewSession(const QString &projectName, const QString &parentDir);
    void closeCurrentSession();

    // 📊 Proyectos Recientes
    QVariantList getRecentProjects() const;
    void addProjectToRecent(const QString &name, const QString &path);

    ARDOUR::Session* session() const { return m_session; }
    ARDOUR::AudioEngine* engine() const { return m_engine; }

signals:
    void sessionInitialized(ARDOUR::Session *session);
    void recentProjectsChanged();

private:
    ARDOUR::Session *m_session = nullptr;
    ARDOUR::AudioEngine *m_engine = nullptr;

    void applySessionSafetyConfig();
};

#endif // NOVASESSIONMANAGER_H