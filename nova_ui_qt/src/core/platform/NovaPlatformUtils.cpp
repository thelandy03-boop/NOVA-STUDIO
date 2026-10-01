#include "NovaPlatformUtils.h"
#include <QStandardPaths>
#include <QDir>
#include <QUrl>
#include <QtGlobal>

QString NovaPlatformUtils::getProjectsDirectory()
{
#if defined(Q_OS_ANDROID)
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
#endif

    if (baseDir.isEmpty()) {
        baseDir = QDir::homePath();
    }

    QString projectsDir = baseDir + "/NovaProjects";
    QDir().mkpath(projectsDir);
    return QDir::cleanPath(projectsDir);
}

QString NovaPlatformUtils::sanitizePath(const QString &rawPath)
{
    if (rawPath.isEmpty()) return QString();

    QString path = rawPath;
    if (path.startsWith("file://")) {
        path = QUrl(path).toLocalFile();
    } else if (path.startsWith("file:/")) {
        path = QUrl(path).toLocalFile();
    }

    path = QUrl::fromPercentEncoding(path.toUtf8());
    return QDir::cleanPath(path);
}

bool NovaPlatformUtils::isMobilePlatform()
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    return true;
#else
    return false;
#endif
}