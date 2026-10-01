#ifndef NOVAPROJECTMANAGER_H
#define NOVAPROJECTMANAGER_H

#include <QObject>
#include <QString>
#include <QVariantList>

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

class NovaProjectManager : public QObject
{
    Q_OBJECT

public:
    explicit NovaProjectManager(QObject *parent = nullptr);

    static QString sanitizePath(const QString &rawPath);
    static bool saveProject(ARDOUR::Session *session);
    static bool isDirty(ARDOUR::Session *session);

    static ARDOUR::Session* createProject(ARDOUR::AudioEngine *engine, const QString &projectName, const QString &parentDir);
    static ARDOUR::Session* openProject(ARDOUR::AudioEngine *engine, const QString &sessionPath);

    static QVariantList getRecentProjects();
    static void addProjectToRecent(const QString &name, const QString &path);
    static void removeProjectFromRecent(const QString &path);

    // 🗑️ Limpia carpetas de proyectos que nunca fueron guardados si el usuario descarta
    static void discardUnsavedProject(ARDOUR::Session *session);
};

#endif // NOVAPROJECTMANAGER_H