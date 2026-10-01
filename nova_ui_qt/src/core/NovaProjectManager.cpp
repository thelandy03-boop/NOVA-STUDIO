#include "NovaProjectManager.h"
#include "NovaLogging.h"
#include "platform/NovaPlatformUtils.h"
#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QSettings>
#include <QDateTime>

#if defined(Q_OS_ANDROID)
#include "platform/NovaAndroidStubs.h"
#else
#pragma push_macro("emit")
#pragma push_macro("slots")
#pragma push_macro("signals")
#pragma push_macro("foreach")
#undef emit
#undef slots
#undef signals
#undef foreach

#include "ardour/session.h"
#include "ardour/audioengine.h"
#include "ardour/session_configuration.h"

#pragma pop_macro("emit")
#pragma pop_macro("slots")
#pragma pop_macro("signals")
#pragma pop_macro("foreach")
#endif

NovaProjectManager::NovaProjectManager(QObject *parent)
    : QObject(parent)
{
}

QString NovaProjectManager::sanitizePath(const QString &rawPath)
{
    return NovaPlatformUtils::sanitizePath(rawPath);
}

bool NovaProjectManager::saveProject(ARDOUR::Session *session)
{
    if (!session) return false;

    try {
        qCDebug(novaCore) << "💾 Guardando estado completo de la sesión...";
        session->set_dirty();
        int result = session->save_state("");
        if (result == 0) {
            QString path = sanitizePath(QString::fromStdString(session->path()));
            QString name = QString::fromStdString(session->name());
            qCDebug(novaCore) << "💾 [OK] Proyecto guardado exitosamente en:" << path;
            
            addProjectToRecent(name, path);
            return true;
        } else {
            qCCritical(novaCore) << "❌ Error al guardar proyecto. Código de retorno:" << result;
        }
    } catch (const std::exception &e) {
        qCCritical(novaCore) << "Excepción crítica durante el guardado:" << e.what();
    }
    return false;
}

bool NovaProjectManager::isDirty(ARDOUR::Session *session)
{
    if (!session) return false;
    return session->dirty();
}

ARDOUR::Session* NovaProjectManager::createProject(ARDOUR::AudioEngine *engine, const QString &projectName, const QString &parentDir)
{
    if (!engine || projectName.isEmpty()) return nullptr;

    QString targetParentDir = parentDir.isEmpty() ? NovaPlatformUtils::getProjectsDirectory() : sanitizePath(parentDir);
    QString sessionDir = targetParentDir + "/" + projectName;
    qCDebug(novaCore) << "🆕 Generando proyecto nuevo independiente en:" << sessionDir;

    try {
        if (QDir(sessionDir).exists()) {
            QDir(sessionDir).removeRecursively();
        }
        QDir().mkpath(targetParentDir);

        ARDOUR::BusProfile bus_profile;
        bus_profile.master_out_channels = 2;

        auto session = new ARDOUR::Session(*engine, sessionDir.toStdString(), projectName.toStdString(), &bus_profile, "", true);

        if (session) {
            engine->set_session(session);
            session->config.set_session_monitoring(ARDOUR::MonitorDisk);

            qCDebug(novaCore) << "🆕 [OK] Nuevo proyecto instanciado correctamente en memoria.";
            return session;
        }
    } catch (const std::exception &e) {
        qCCritical(novaCore) << "❌ Excepción durante la creación del proyecto:" << e.what();
    }
    return nullptr;
}

ARDOUR::Session* NovaProjectManager::openProject(ARDOUR::AudioEngine *engine, const QString &sessionPath)
{
    if (!engine) return nullptr;

    QString cleanSessionPath = sanitizePath(sessionPath);
    qCDebug(novaCore) << "📂 Intentando cargar proyecto existente en:" << cleanSessionPath;

    try {
        QFileInfo checkFile(cleanSessionPath);
        QString realSessionDir = cleanSessionPath;
        QString sessionName = checkFile.fileName();

        if (checkFile.isFile() && checkFile.suffix() == "ardour") {
            realSessionDir = checkFile.absolutePath();
            sessionName = checkFile.completeBaseName();
        } else if (checkFile.isDir()) {
            realSessionDir = checkFile.absoluteFilePath();
            sessionName = checkFile.fileName();
        } else {
            sessionName = QDir(cleanSessionPath).dirName();
        }

        realSessionDir = sanitizePath(realSessionDir);

        QString expectedArdourFile = realSessionDir + "/" + sessionName + ".ardour";
        if (!QFileInfo::exists(expectedArdourFile)) {
            QDir dir(realSessionDir);
            QStringList ardourFiles = dir.entryList(QStringList() << "*.ardour", QDir::Files);
            if (!ardourFiles.isEmpty()) {
                sessionName = QFileInfo(ardourFiles.first()).completeBaseName();
            }
        }

        std::string pathStd = realSessionDir.toStdString();
        std::string nameStd = sessionName.toStdString();

        ARDOUR::BusProfile bus_profile;
        bus_profile.master_out_channels = 2;

        auto session = new ARDOUR::Session(*engine, pathStd, nameStd, &bus_profile, "", false);

        if (session) {
            engine->set_session(session);
            session->config.set_session_monitoring(ARDOUR::MonitorDisk);

            qCDebug(novaCore) << "📂 [OK] Proyecto cargado con éxito:" << realSessionDir << "Snapshot:" << sessionName;
            addProjectToRecent(sessionName, realSessionDir);
            return session;
        }
    } catch (const std::exception &e) {
        qCCritical(novaCore) << "❌ Error al abrir la sesión:" << e.what();
    }
    return nullptr;
}

QVariantList NovaProjectManager::getRecentProjects()
{
    QSettings settings("NovaStudio", "DAW");
    QVariantList rawList = settings.value("recentProjects").toList();
    QVariantList cleanedList;

    for (const auto &item : rawList) {
        QVariantMap map = item.toMap();
        QString cleanP = sanitizePath(map["path"].toString());
        if (!cleanP.isEmpty() && QDir(cleanP).exists()) {
            map["path"] = cleanP;
            cleanedList.append(map);
        }
    }
    return cleanedList;
}

void NovaProjectManager::addProjectToRecent(const QString &name, const QString &path)
{
    if (name.isEmpty() || name == "TempSession" || name == "DefaultSession" || path.contains("NovaStudio_TempSession")) {
        return;
    }

    QString cleanPath = sanitizePath(path);
    QSettings settings("NovaStudio", "DAW");
    QVariantList list = settings.value("recentProjects").toList();
    QVariantList newList;

    for (int i = 0; i < list.size(); ++i) {
        QVariantMap item = list[i].toMap();
        QString itemPath = sanitizePath(item["path"].toString());
        if (itemPath != cleanPath && !itemPath.isEmpty()) {
            newList.append(item);
        }
    }

    QVariantMap projectMap;
    projectMap["name"] = name;
    projectMap["path"] = cleanPath;
    projectMap["lastModified"] = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm AP");

    newList.prepend(projectMap);

    while (newList.size() > 10) {
        newList.removeLast();
    }

    settings.setValue("recentProjects", newList);
    qCDebug(novaCore) << "📊 Historial de Proyectos Recientes Actualizado:" << name << "->" << cleanPath;
}

void NovaProjectManager::removeProjectFromRecent(const QString &path)
{
    QString cleanTarget = sanitizePath(path);
    QSettings settings("NovaStudio", "DAW");
    QVariantList list = settings.value("recentProjects").toList();
    QVariantList newList;

    for (int i = 0; i < list.size(); ++i) {
        QVariantMap item = list[i].toMap();
        QString itemPath = sanitizePath(item["path"].toString());
        if (itemPath != cleanTarget && !itemPath.isEmpty()) {
            newList.append(item);
        }
    }

    settings.setValue("recentProjects", newList);
    qCDebug(novaCore) << "🗑️ Proyecto eliminado del historial de recientes:" << cleanTarget;
}

void NovaProjectManager::discardUnsavedProject(ARDOUR::Session *session)
{
    if (!session) return;

    QString path = sanitizePath(QString::fromStdString(session->path()));
    removeProjectFromRecent(path);

    if (!path.isEmpty() && QDir(path).exists()) {
        qCDebug(novaCore) << "🗑️ Descartando y eliminando carpeta física de proyecto no guardado:" << path;
        QDir(path).removeRecursively();
    }
}