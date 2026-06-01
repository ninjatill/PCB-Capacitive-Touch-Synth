#include <stdio.h>

#include "voice_manager.h"
#include "../config/firmware_config.h"

static uint8_t current_voice = 0;

bool voice_manager_init()
{
    printf("Initializing voice manager...\n");

    current_voice = 0;

    printf("Voice manager initialized. current_voice=%u\n", current_voice);

    return true;
}

uint8_t voice_manager_get_current()
{
    return current_voice;
}

void voice_manager_set_current(uint8_t voice)
{
    if (voice >= VOICE_COUNT) {
        voice = 0;
    }

    current_voice = voice;

    printf("VOICE: current voice=%u%s\n",
           current_voice,
           voice_manager_is_recorded_voice(current_voice) ? " recorded" : "");
}

uint8_t voice_manager_next()
{
    uint8_t next_voice = current_voice + 1;

    if (next_voice >= VOICE_COUNT) {
        next_voice = 0;
    }

    voice_manager_set_current(next_voice);

    return current_voice;
}

bool voice_manager_is_recorded_voice(uint8_t voice)
{
    return voice == VOICE_RECORD_PLAYBACK;
}