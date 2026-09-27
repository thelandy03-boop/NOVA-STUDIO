#ifndef NOVATIMEUTILS_H
#define NOVATIMEUTILS_H

#include <QString>

class NovaTimeUtils
{
public:
    // 📐 Conversiones Beat <-> Pixel
    static double beatToPixel(double beat, double barWidth = 80.0);
    static double pixelToBeat(double pixelX, double barWidth = 80.0);

    // ⏱️ Conversiones Frame <-> Beat
    static double frameToBeat(double frame, double sampleRate = 44100.0, double bpm = 120.0);
    static double beatToFrame(double beat, double sampleRate = 44100.0, double bpm = 120.0);

    // 🎼 Formateadores de Texto
    static QString formatBBT(double beat);
    static QString formatTimecode(double frame, double sampleRate = 44100.0);
};

#endif // NOVATIMEUTILS_H