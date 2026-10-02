#pragma once

#include <cmath>
#include <algorithm>

namespace NovaAudioUtils {

    /**
     * @brief Convierte amplitud lineal (coeficiente de fader) a Decibelios (dB).
     * @param coeff Amplitud en rango [0.0, 1.0+]
     * @return Nivel en dB. El silencio absoluto se limita a -192 dB.
     */
    [[nodiscard]] inline float coeffToDb(float coeff) noexcept
    {
        if (coeff <= 0.0000001f) {
            return -192.0f;
        }
        return 20.0f * std::log10(coeff);
    }

    /**
     * @brief Convierte Decibelios (dB) a amplitud lineal (coeficiente de fader).
     * @param dB Nivel en decibelios.
     * @return Coeficiente de amplitud en rango [0.0, 1.0+]
     */
    [[nodiscard]] inline float dbToCoeff(float dB) noexcept
    {
        if (dB <= -192.0f) {
            return 0.0f;
        }
        return std::pow(10.0f, dB / 20.0f);
    }

    /**
     * @brief Algoritmo de Soft Clipping analógico.
     * Limita de forma no lineal las muestras que exceden los -2 dBFS (0.8f) para simular
     * la saturación cálida de bulbos/cinta en lugar de la distorsión digital dura (hard clipping).
     * @param x Muestra de audio de entrada.
     * @return Muestra procesada y limitada.
     */
    [[nodiscard]] inline float applySoftClip(float x) noexcept
    {
        constexpr float threshold = 0.8f;
        constexpr float margin = 0.2f;

        if (x > threshold) {
            return threshold + margin * std::tanh((x - threshold) / margin);
        } else if (x < -threshold) {
            return -(threshold + margin * std::tanh((-x - threshold) / margin));
        }
        return x;
    }

} // namespace NovaAudioUtils