#include <stdio.h>

#include "voice_manager.h"
#include "voice_definitions.h"
#include "../config/firmware_config.h"

static bool    voice_initialized = false;
static uint8_t current_voice     = 0;
static uint8_t current_bank      = 0;

bool voice_manager_initialized()
{
    return voice_initialized;
}

bool voice_manager_init()
{
    printf("Initializing voice manager...\n");

    current_voice = 0;

    voice_initialized = true;

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
    // Advance to the next assigned voice slot in the current bank,
    // skipping any unassigned slots.  Wraps from the last slot back
    // to slot 0 and continues scanning to find the first assigned one.
    uint8_t next = (current_voice + 1) % VOICE_COUNT;
    for (uint8_t attempts = 0; attempts < VOICE_COUNT; attempts++) {
        if (voice_definitions_get(next) != nullptr) {
            voice_manager_set_current(next);
            return current_voice;
        }
        next = (next + 1) % VOICE_COUNT;
    }
    // No slots assigned at all — stay on current voice.
    return current_voice;
}

uint8_t voice_manager_get_bank()
{
    return current_bank;
}

void voice_manager_set_bank(uint8_t bank)
{
    uint8_t configured = voice_definitions_configured_bank_count();
    if (configured == 0) configured = 1;
    if (bank >= configured) bank = 0;

    current_bank = bank;
    voice_definitions_set_bank(bank);

    // When switching banks, move to the first assigned voice in the new bank.
    // voice_definitions_get() now looks at the newly active bank.
    uint8_t first = 0;
    for (uint8_t v = 0; v < VOICE_COUNT; v++) {
        if (voice_definitions_get(v) != nullptr) {
            first = v;
            break;
        }
    }
    current_voice = first;

    printf("VOICE: bank=%u voice=%u\n", current_bank, current_voice);
}

uint8_t voice_manager_next_bank()
{
    uint8_t configured = voice_definitions_configured_bank_count();
    if (configured <= 1) return current_bank;  // nothing to cycle

    uint8_t next = (current_bank + 1) % configured;
    voice_manager_set_bank(next);
    return current_bank;
}

bool voice_manager_is_recorded_voice(uint8_t voice)
{
    return voice == VOICE_RECORD_PLAYBACK;
}