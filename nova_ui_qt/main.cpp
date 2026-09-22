#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QUrl>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QDirIterator>
#include <QDebug>
#include <QTimer>

int main(int argc, char *argv[]) {
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");

    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    // Ruta absoluta a tus archivos de desarrollo QML
    QString qmlDir = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../../qml");
    QString mainQml = qmlDir + "/Main.qml";

    if (QFileInfo::exists(mainQml)) {
        qDebug() << "🔥 MODO DESARROLLO (Hot Reload Activo) desde:" << qmlDir;

        engine.addImportPath(qmlDir);
        QUrl url = QUrl::fromLocalFile(mainQml);

        // 1. Instanciar en el Heap pasando &app para asegurar persistencia
        auto watcher = new QFileSystemWatcher(&app);

        // Función para registrar tanto archivos como carpetas (evita el problema del guardado atómico en IDEs)
        auto addAllPaths = [watcher, qmlDir]() {
            if (!watcher->files().isEmpty()) watcher->removePaths(watcher->files());
            if (!watcher->directories().isEmpty()) watcher->removePaths(watcher->directories());

            watcher->addPath(qmlDir); // Monitorear el directorio principal

            QDirIterator it(qmlDir, QStringList() << "*.qml" << "qmldir", QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                QString path = it.next();
                watcher->addPath(path);
                watcher->addPath(QFileInfo(path).absolutePath()); // Añadir subcarpetas
            }
        };

        addAllPaths();

        // Evitar múltiples disparos consecutivos al guardar (Debounce)
        auto reloadTimer = new QTimer(&app);
        reloadTimer->setSingleShot(true);
        reloadTimer->setInterval(150); // 150 ms de espera tras el guardado

        auto triggerReload = [&engine, url, addAllPaths, reloadTimer](const QString &path) {
            qDebug() << "⚡ Modificación detectada en:" << path;
            
            // Reiniciar timer
            reloadTimer->disconnect();
            QObject::connect(reloadTimer, &QTimer::timeout, [&engine, url, addAllPaths]() {
                qDebug() << "🔄 Ejecutando Hot Reload...";
                engine.clearComponentCache();

                for (auto obj : engine.rootObjects()) {
                    delete obj;
                }

                addAllPaths(); // Volver a vincular los observadores de archivos
                engine.load(url);
            });

            reloadTimer->start();
        };

        // Escuchar tanto cambios en archivos como en directorios
        QObject::connect(watcher, &QFileSystemWatcher::fileChanged, triggerReload);
        QObject::connect(watcher, &QFileSystemWatcher::directoryChanged, triggerReload);

        engine.load(url);
    } else {
        engine.addImportPath("qrc:/qt/qml/NovaStudio");
        engine.load(QUrl(QStringLiteral("qrc:/qt/qml/NovaStudio/qml/Main.qml")));
    }

    return app.exec();
}