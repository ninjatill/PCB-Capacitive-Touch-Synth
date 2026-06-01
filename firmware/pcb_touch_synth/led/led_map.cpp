#include "led_map.h"
#include "../config/i2c_addresses.h"

const LedMap LED_MAP[] = {

    // ==================================================
    // OCTAVE INDICATOR LEDS
    // ==================================================

    { I2C_ADDR_LED_1, 15, LED_ROLE_OCTAVE, 0, "LED_OCTAVE_0" },
    { I2C_ADDR_LED_1, 14, LED_ROLE_OCTAVE, 1, "LED_OCTAVE_1" },
    { I2C_ADDR_LED_1, 13, LED_ROLE_OCTAVE, 2, "LED_OCTAVE_2" },
    { I2C_ADDR_LED_1, 12, LED_ROLE_OCTAVE, 3, "LED_OCTAVE_3" },
    { I2C_ADDR_LED_1, 11, LED_ROLE_OCTAVE, 4, "LED_OCTAVE_4" },

    // ==================================================
    // VOICE BINARY INDICATOR LEDS
    // 1, 2, 4, 8
    // ==================================================

    { I2C_ADDR_LED_2, 3, LED_ROLE_VOICE, 0, "LED_VOICE_1" },
    { I2C_ADDR_LED_2, 2, LED_ROLE_VOICE, 1, "LED_VOICE_2" },
    { I2C_ADDR_LED_2, 1, LED_ROLE_VOICE, 2, "LED_VOICE_4" },
    { I2C_ADDR_LED_2, 0, LED_ROLE_VOICE, 3, "LED_VOICE_8" },

    // ==================================================
    // STATUS / MODE LEDS
    // ==================================================

    { I2C_ADDR_LED_2, 13, LED_ROLE_STATUS, 2, "LED_RECORD" },
    { I2C_ADDR_LED_2, 15, LED_ROLE_STATUS, 0, "LED_MODE" },
    { I2C_ADDR_LED_2, 14, LED_ROLE_STATUS, 1, "LED_MIDI_LR" },

    // ==================================================
    // NOTE LEDS
    // Placeholder sequential map.
    // Update controller/channel values to match PCB routing.
    // ==================================================

    { I2C_ADDR_LED_1,  0, LED_ROLE_NOTE,  0, "LED_F3"  },
    { I2C_ADDR_LED_1,  1, LED_ROLE_NOTE,  1, "LED_FS3" },
    { I2C_ADDR_LED_1,  2, LED_ROLE_NOTE,  2, "LED_G3"  },
    { I2C_ADDR_LED_1,  3, LED_ROLE_NOTE,  3, "LED_GS3" },
    { I2C_ADDR_LED_1,  4, LED_ROLE_NOTE,  4, "LED_A3"  },
    { I2C_ADDR_LED_1,  5, LED_ROLE_NOTE,  5, "LED_AS3" },
    { I2C_ADDR_LED_1,  6, LED_ROLE_NOTE,  6, "LED_B3"  },
    { I2C_ADDR_LED_1,  7, LED_ROLE_NOTE,  7, "LED_C4"  },
    { I2C_ADDR_LED_1,  8, LED_ROLE_NOTE,  8, "LED_CS4" },
    { I2C_ADDR_LED_1,  9, LED_ROLE_NOTE,  9, "LED_D4"  },
    { I2C_ADDR_LED_1, 10, LED_ROLE_NOTE, 10, "LED_DS4" },
    { I2C_ADDR_LED_2,  4, LED_ROLE_NOTE, 11, "LED_E4"  },
    { I2C_ADDR_LED_2,  5, LED_ROLE_NOTE, 12, "LED_F4"  },
    { I2C_ADDR_LED_2,  6, LED_ROLE_NOTE, 13, "LED_FS4" },
    { I2C_ADDR_LED_2,  7, LED_ROLE_NOTE, 14, "LED_G4"  },
    { I2C_ADDR_LED_2,  8, LED_ROLE_NOTE, 15, "LED_GS4" },

    { I2C_ADDR_LED_2,  9, LED_ROLE_NOTE, 16, "LED_A4"  },
    { I2C_ADDR_LED_2, 10, LED_ROLE_NOTE, 17, "LED_AS4" },
    { I2C_ADDR_LED_2, 11, LED_ROLE_NOTE, 18, "LED_B4"  },
    { I2C_ADDR_LED_2, 12, LED_ROLE_NOTE, 19, "LED_C5"  }
};

const uint8_t LED_MAP_COUNT =
    sizeof(LED_MAP) / sizeof(LED_MAP[0]);