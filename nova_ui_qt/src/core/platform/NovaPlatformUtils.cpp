#include "NovaPlatformUtils.h"
#include <QStandardPaths>
#include <QDir>
#include <QUrl>
#include <QtGlobal>

QString NovaPlatformUtils::getProjectsDirectory()
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_WIN)
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

    // 1. Primero decodificar percent-encoding (resuelve %20, %2F, %3A, etc.)
    QString path = QUrl::fromPercentEncoding(rawPath.toUtf8());

    // 2. Si contiene el esquema "file:", extraer la ruta local absoluta de forma robusta
    if (path.startsWith("file:", Qt::CaseInsensitive)) {
        QUrl url(path);
        if (url.isValid() && url.isLocalFile()) {
            path = url.toLocalFile();
        } else {
            // Limpieza manual de seguridad si el esquema de QUrl falla
            path.remove(0, 5); // Quita "file:"
            
            // Reemplazar diagonales dobles iniciales de esquemas como file:// o file:///
            while (path.startsWith("//")) {
                path.remove(0, 1);
            }
            
            // Garantizar que en Linux/Android empiece con un solo "/"
#if !defined(Q_OS_WIN)
            if (!path.startsWith("/")) {
                path = "/" + path;
            }
#endif
        }
    }

    // 3. Normalizar la ruta final del sistema de archivos
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