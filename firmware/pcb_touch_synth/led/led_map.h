#pragma once

#include <stdint.h>

enum LedRole {
    LED_ROLE_NOTE,
    LED_ROLE_OCTAVE,
    LED_ROLE_VOICE,
    LED_ROLE_STATUS
};

struct LedMap {
    uint8_t controller_addr;
    uint8_t channel;
    LedRole role;
    uint8_t index;
    const char* name;
};

enum StatusLed {
    STATUS_LED_RECORD = 0,
    STATUS_LED_MODE = 1,
    STATUS_LED_MIDI_LR = 2
};

enum VoiceLed {
    VOICE_LED_1 = 1,
    VOICE_LED_2 = 2,
    VOICE_LED_4 = 4,
    VOICE_LED_8 = 8
};

enum OctaveLed {
    OCTAVE_LED_0 = 0,
    OCTAVE_LED_1 = 1,
    OCTAVE_LED_2 = 2,
    OCTAVE_LED_3 = 3,
    OCTAVE_LED_4 = 4
};

extern const LedMap LED_MAP[];
extern const uint8_t LED_MAP_COUNT;