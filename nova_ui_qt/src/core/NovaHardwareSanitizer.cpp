#include "NovaHardwareSanitizer.h"
#include "NovaLogging.h"
#include <QDebug>
#include <QtGlobal>

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID) || defined(Q_OS_WIN) && !defined(Q_OS_WIN)
#include <alsa/asoundlib.h>
#endif

void NovaHardwareSanitizer::sanitize()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID) || defined(Q_OS_WIN) && !defined(Q_OS_WIN)
    qCDebug(novaCore) << "🎙️ [Sanitizer] Verificando aislamiento de hardware ALSA...";

    int cardNum = -1;
    while (snd_card_next(&cardNum) == 0 && cardNum >= 0) {
        char cardName[32];
        snprintf(cardName, sizeof(cardName), "hw:%d", cardNum);
        
        snd_mixer_t* mixer = nullptr;
        if (snd_mixer_open(&mixer, 0) < 0) continue;
        if (snd_mixer_attach(mixer, cardName) < 0) { snd_mixer_close(mixer); continue; }
        if (snd_mixer_selem_register(mixer, nullptr, nullptr) < 0) { snd_mixer_close(mixer); continue; }
        if (snd_mixer_load(mixer) < 0) { snd_mixer_close(mixer); continue; }
        
        for (snd_mixer_elem_t* elem = snd_mixer_first_elem(mixer); elem != nullptr; elem = snd_mixer_elem_next(elem)) {
            if (!snd_mixer_selem_is_active(elem)) continue;
            
            const char* name = snd_mixer_selem_get_name(elem);
            std::string sName(name);
            
            // 🚫 Silenciar únicamente el retorno directo de auriculares (Side-tone)
            if (sName == "Mic" || sName == "Internal Mic") {
                if (snd_mixer_selem_has_playback_volume(elem)) {
                    snd_mixer_selem_set_playback_volume_all(elem, 0);
                }
                if (snd_mixer_selem_has_playback_switch(elem)) {
                    snd_mixer_selem_set_playback_switch_all(elem, 0);
                }
            }
            // 🚫 Desactivar Loopback Mixing analógico de silicio
            else if (sName == "Loopback Mixing") {
                if (snd_mixer_selem_get_enum_items(elem) > 0) {
                    int items = snd_mixer_selem_get_enum_items(elem);
                    for (int i = 0; i < items; ++i) {
                        char itemName[64];
                        if (snd_mixer_selem_get_enum_item_name(elem, i, sizeof(itemName), itemName) >= 0) {
                            if (std::string(itemName) == "Disabled" || std::string(itemName) == "Off") {
                                snd_mixer_selem_set_enum_item(elem, (snd_mixer_selem_channel_id_t)0, i);
                                break;
                            }
                        }
                    }
                }
            }
        }
        snd_mixer_close(mixer);
    }
#endif
}