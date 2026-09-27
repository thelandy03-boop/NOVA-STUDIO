#include "NovaTimeUtils.h"
#include <algorithm>
#include <cmath>

double NovaTimeUtils::beatToPixel(double beat, double barWidth)
{
    double pixelsPerBeat = barWidth / 4.0; // Firma 4/4
    return std::max(0.0, beat) * pixelsPerBeat;
}

double NovaTimeUtils::pixelToBeat(double pixelX, double barWidth)
{
    if (barWidth <= 0.0) return 0.0;
    double pixelsPerBeat = barWidth / 4.0;
    return std::max(0.0, pixelX / pixelsPerBeat);
}

double NovaTimeUtils::frameToBeat(double frame, double sampleRate, double bpm)
{
    if (sampleRate <= 0.0 || bpm <= 0.0) return 0.0;
    double seconds = std::max(0.0, frame) / sampleRate;
    return seconds * (bpm / 60.0);
}

double NovaTimeUtils::beatToFrame(double beat, double sampleRate, double bpm)
{
    if (bpm <= 0.0) return 0.0;
    double seconds = std::max(0.0, beat) * (60.0 / bpm);
    return seconds * (sampleRate > 0.0 ? sampleRate : 44100.0);
}

QString NovaTimeUtils::formatBBT(double beat)
{
    double validBeat = std::max(0.0, beat);
    int bar = 1 + static_cast<int>(validBeat / 4.0);
    int b = 1 + static_cast<int>(std::fmod(validBeat, 4.0));
    int sub = 1 + static_cast<int>(std::fmod(validBeat * 4.0, 4.0));

    return QString("%1 : %2 : %3")
            .arg(bar, 3, 10, QChar('0'))
            .arg(b, 2, 10, QChar('0'))
            .arg(sub, 2, 10, QChar('0'));
}

QString NovaTimeUtils::formatTimecode(double frame, double sampleRate)
{
    double sr = sampleRate > 0.0 ? sampleRate : 44100.0;
    double currentSeconds = std::max(0.0, frame) / sr;

    int totalSeconds = static_cast<int>(currentSeconds);
    int mins = totalSeconds / 60;
    int secs = totalSeconds % 60;
    int ms = static_cast<int>((frame / (sr / 100.0))) % 100;

    return QString("%1:%2:%3")
            .arg(mins, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'))
            .arg(ms, 2, 10, QChar('0'));
}