#include "led_map.h"
#include "led_animation.h"
#include "../config/i2c_addresses.h"

const LedMap LED_MAP[] = {

    // ==================================================
    // OCTAVE INDICATOR LEDS
    // ==================================================

    { I2C_ADDR_LED_1, 15, LED_ROLE_OCTAVE, 0, "LED_OCTAVE_0", 110.0f, 99.0f },
    { I2C_ADDR_LED_1, 14, LED_ROLE_OCTAVE, 1, "LED_OCTAVE_1", 115.0f, 99.0f },
    { I2C_ADDR_LED_1, 13, LED_ROLE_OCTAVE, 2, "LED_OCTAVE_2", 120.0f, 99.0f },
    { I2C_ADDR_LED_1, 12, LED_ROLE_OCTAVE, 3, "LED_OCTAVE_3", 125.0f, 99.0f },
    { I2C_ADDR_LED_1, 11, LED_ROLE_OCTAVE, 4, "LED_OCTAVE_4", 130.0f, 99.0f },

    // ==================================================
    // VOICE BINARY INDICATOR LEDS
    // These 4 LEDs show the current voice (0-15) in binary.
    // The 'index' field must match the VoiceLed enum values (1, 2, 4, 8)
    // so that led_manager_set_voice() can locate each LED via find_led_index().
    // ==================================================

    { I2C_ADDR_LED_2, 3, LED_ROLE_VOICE, VOICE_LED_1, "LED_VOICE_1", 109.9f, 83.0f },
    { I2C_ADDR_LED_2, 2, LED_ROLE_VOICE, VOICE_LED_2, "LED_VOICE_2", 116.6f, 83.0f },
    { I2C_ADDR_LED_2, 1, LED_ROLE_VOICE, VOICE_LED_4, "LED_VOICE_4", 123.4f, 83.0f },
    { I2C_ADDR_LED_2, 0, LED_ROLE_VOICE, VOICE_LED_8, "LED_VOICE_8", 130.1f, 83.0f },

    // ==================================================
    // STATUS / MODE LEDS
    // ==================================================

    { I2C_ADDR_LED_2, 13, LED_ROLE_STATUS, 2, "LED_RECORD",  170.5f, 84.0f },
    { I2C_ADDR_LED_2, 15, LED_ROLE_STATUS, 0, "LED_MODE",    172.0f, 99.0f },
    { I2C_ADDR_LED_2, 14, LED_ROLE_STATUS, 1, "LED_MIDI_LR", 183.3f, 99.0f },

    // ==================================================
    // NOTE LEDS
    // Placeholder sequential map.
    // Update controller/channel values to match PCB routing.
    // ==================================================

    { I2C_ADDR_LED_1,  0, LED_ROLE_NOTE,  0, "LED_F3",   13.5f, 28.5f },
    { I2C_ADDR_LED_1,  1, LED_ROLE_NOTE,  1, "LED_FS3",  25.0f, 52.0f },
    { I2C_ADDR_LED_1,  2, LED_ROLE_NOTE,  2, "LED_G3",   36.5f, 28.5f },
    { I2C_ADDR_LED_1,  3, LED_ROLE_NOTE,  3, "LED_GS3",  48.0f, 52.0f },
    { I2C_ADDR_LED_1,  4, LED_ROLE_NOTE,  4, "LED_A3",   59.5f, 28.5f },
    { I2C_ADDR_LED_1,  5, LED_ROLE_NOTE,  5, "LED_AS3",  71.0f, 52.0f },
    { I2C_ADDR_LED_1,  6, LED_ROLE_NOTE,  6, "LED_B3",   82.5f, 28.5f },
    { I2C_ADDR_LED_1,  7, LED_ROLE_NOTE,  7, "LED_C4",  105.5f, 28.5f },
    { I2C_ADDR_LED_1,  8, LED_ROLE_NOTE,  8, "LED_CS4", 117.0f, 52.0f },
    { I2C_ADDR_LED_1,  9, LED_ROLE_NOTE,  9, "LED_D4",  128.5f, 28.5f },
    { I2C_ADDR_LED_1, 10, LED_ROLE_NOTE, 10, "LED_DS4", 140.0f, 52.0f },
    { I2C_ADDR_LED_2,  4, LED_ROLE_NOTE, 11, "LED_E4",  151.5f, 28.5f },
    { I2C_ADDR_LED_2,  5, LED_ROLE_NOTE, 12, "LED_F4",  174.5f, 28.5f },
    { I2C_ADDR_LED_2,  6, LED_ROLE_NOTE, 13, "LED_FS4", 186.0f, 52.0f },
    { I2C_ADDR_LED_2,  7, LED_ROLE_NOTE, 14, "LED_G4",  197.5f, 28.5f },
    { I2C_ADDR_LED_2,  8, LED_ROLE_NOTE, 15, "LED_GS4", 209.0f, 52.0f },

    { I2C_ADDR_LED_2,  9, LED_ROLE_NOTE, 16, "LED_A4",  220.5f, 28.5f },
    { I2C_ADDR_LED_2, 10, LED_ROLE_NOTE, 17, "LED_AS4", 232.0f, 52.0f },
    { I2C_ADDR_LED_2, 11, LED_ROLE_NOTE, 18, "LED_B4",  243.5f, 28.5f },
    { I2C_ADDR_LED_2, 12, LED_ROLE_NOTE, 19, "LED_C5",  266.5f, 28.5f }
};

const uint8_t LED_MAP_COUNT =
    sizeof(LED_MAP) / sizeof(LED_MAP[0]);