#pragma once

#include <QObject>

class NovaHardwareSanitizer
{
public:
    // 🎙️ Sanitiza programáticamente todas las tarjetas físicas ALSA al arrancar (Side-tone & Loopback off)
    static void sanitize();
};