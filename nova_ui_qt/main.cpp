#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QDirIterator>
#include <QDebug>
#include <QTimer>
#include <QWindow>

#include "src/NovaAudioEngine.h"

int main(int argc, char *argv[])
{
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    qputenv("QML_DISABLE_DISK_CACHE", "1");

    QGuiApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    // Instancia del Motor de Audio Nova / Ardour Bridge
    NovaAudioEngine audioEngine;
    audioEngine.initEngine();

    QQmlApplicationEngine engine;

    // Registrar audioEngine globalmente en QML
    engine.rootContext()->setContextProperty("AudioEngine", &audioEngine);

    // build/ → ../qml  |  build/Release → ../../qml
    QStringList candidates;
    candidates << QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../qml")
               << QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../../qml")
               << QDir::cleanPath(QDir::currentPath() + "/../qml")
               << QDir::cleanPath(QDir::currentPath() + "/qml");

    QString qmlDir;
    QString mainQml;
    for (const QString &c : candidates) {
        const QString m = c + "/Main.qml";
        if (QFileInfo::exists(m)) {
            qmlDir = c;
            mainQml = m;
            break;
        }
    }

    if (!mainQml.isEmpty()) {
        qDebug() << "HOT-RELOAD activo desde:" << qmlDir;

        engine.addImportPath(qmlDir);
        engine.addImportPath(qmlDir + "/skins");
        engine.addImportPath(qmlDir + "/skins/draft");
        engine.addImportPath(qmlDir + "/skins/bandlab");
        engine.addImportPath(qmlDir + "/skins/reaper");

        const QUrl url = QUrl::fromLocalFile(mainQml);

        auto *watcher = new QFileSystemWatcher(&app);
        auto *reloadTimer = new QTimer(&app);
        reloadTimer->setSingleShot(true);
        reloadTimer->setInterval(200);

        auto addAllPaths = [watcher, qmlDir]() {
            const auto files = watcher->files();
            if (!files.isEmpty()) watcher->removePaths(files);
            const auto dirs = watcher->directories();
            if (!dirs.isEmpty()) watcher->removePaths(dirs);

            watcher->addPath(qmlDir);
            QDirIterator it(qmlDir,
                            QStringList() << "*.qml" << "qmldir" << "*.svg",
                            QDir::Files,
                            QDirIterator::Subdirectories);
            while (it.hasNext()) {
                const QString path = it.next();
                watcher->addPath(path);
                watcher->addPath(QFileInfo(path).absolutePath());
            }
        };

        addAllPaths();

        auto doReload = [&engine, url, addAllPaths, &audioEngine]() {
            qDebug() << "HOT-RELOAD ejecutando...";

            const auto roots = engine.rootObjects();
            for (QObject *obj : roots) {
                if (auto *w = qobject_cast<QWindow *>(obj))
                    w->hide();
                obj->deleteLater();
            }

            engine.clearComponentCache();
            // Mantener el contexto de AudioEngine tras la recarga
            engine.rootContext()->setContextProperty("AudioEngine", &audioEngine);
            addAllPaths();

            QTimer::singleShot(0, &engine, [&engine, url]() {
                engine.load(url);
                if (engine.rootObjects().isEmpty())
                    qCritical() << "HOT-RELOAD: fallo al recargar Main.qml";
                else
                    qDebug() << "HOT-RELOAD OK";
            });
        };

        QObject::connect(reloadTimer, &QTimer::timeout, &app, doReload);

        auto scheduleReload = [reloadTimer](const QString &path) {
            qDebug() << "QML cambio:" << path;
            reloadTimer->start();
        };

        QObject::connect(watcher, &QFileSystemWatcher::fileChanged, &app, scheduleReload);
        QObject::connect(watcher, &QFileSystemWatcher::directoryChanged, &app, scheduleReload);

        engine.load(url);
        if (engine.rootObjects().isEmpty()) {
            qCritical() << "No se pudo cargar" << mainQml;
            return -1;
        }
    } else {
        qDebug() << "Modo QRC (sin hot-reload de fuentes)";
        engine.addImportPath(QStringLiteral("qrc:/qt/qml/NovaStudio"));
        engine.load(QUrl(QStringLiteral("qrc:/qt/qml/NovaStudio/qml/Main.qml")));
        if (engine.rootObjects().isEmpty())
            return -1;
    }

    return app.exec();
}