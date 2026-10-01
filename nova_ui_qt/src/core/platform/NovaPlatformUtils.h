#ifndef NOVAPLATFORMUTILS_H
#define NOVAPLATFORMUTILS_H

#include <QString>

class NovaPlatformUtils
{
public:
    // 📁 Devuelve la ruta de carpetas de proyectos según el sistema operativo (Desktop vs Android)
    static QString getProjectsDirectory();

    // 🧹 Sanitizador de rutas unificado (maneja file://, file:/ y URL encoding)
    static QString sanitizePath(const QString &rawPath);

    // 📱 Detecta si el binario actual está ejecutándose en Android o móvil
    static bool isMobilePlatform();
};

#endif // NOVAPLATFORMUTILS_H