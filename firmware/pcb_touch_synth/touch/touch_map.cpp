#include "touch_map.h"
#include "../config/i2c_addresses.h"

const TouchPadMap TOUCH_MAP[] = {

    // ==================================================
    // TOUCH CONTROLLER 1 (0x20)
    // Physical notes F3 -> E4
    // ==================================================

    { I2C_ADDR_TOUCH_1,  6, TOUCH_ROLE_NOTE,    0, 53, TOUCH_CONTROL_NONE, "F3"  },
    { I2C_ADDR_TOUCH_1,  5, TOUCH_ROLE_NOTE,    1, 54, TOUCH_CONTROL_NONE, "F#3" },
    { I2C_ADDR_TOUCH_1,  4, TOUCH_ROLE_NOTE,    2, 55, TOUCH_CONTROL_NONE, "G3"  },
    { I2C_ADDR_TOUCH_1,  3, TOUCH_ROLE_NOTE,    3, 56, TOUCH_CONTROL_NONE, "G#3" },
    { I2C_ADDR_TOUCH_1,  2, TOUCH_ROLE_NOTE,    4, 57, TOUCH_CONTROL_NONE, "A3"  },
    { I2C_ADDR_TOUCH_1,  1, TOUCH_ROLE_NOTE,    5, 58, TOUCH_CONTROL_NONE, "A#3" },
    { I2C_ADDR_TOUCH_1,  0, TOUCH_ROLE_NOTE,    6, 59, TOUCH_CONTROL_NONE, "B3"  },
    { I2C_ADDR_TOUCH_1, 11, TOUCH_ROLE_NOTE,    7, 60, TOUCH_CONTROL_NONE, "C4"  },
    { I2C_ADDR_TOUCH_1, 10, TOUCH_ROLE_NOTE,    8, 61, TOUCH_CONTROL_NONE, "C#4" },
    { I2C_ADDR_TOUCH_1,  9, TOUCH_ROLE_NOTE,    9, 62, TOUCH_CONTROL_NONE, "D4"  },
    { I2C_ADDR_TOUCH_1,  8, TOUCH_ROLE_NOTE,   10, 63, TOUCH_CONTROL_NONE, "D#4" },
    { I2C_ADDR_TOUCH_1,  7, TOUCH_ROLE_NOTE,   11, 64, TOUCH_CONTROL_NONE, "E4"  },

    // ==================================================
    // TOUCH CONTROLLER 2 (0x21)
    // Physical notes F4 -> C5
    // ==================================================

    { I2C_ADDR_TOUCH_2,  6, TOUCH_ROLE_NOTE,   12, 65, TOUCH_CONTROL_NONE, "F4"  },
    { I2C_ADDR_TOUCH_2, 11, TOUCH_ROLE_NOTE,   13, 66, TOUCH_CONTROL_NONE, "F#4" },
    { I2C_ADDR_TOUCH_2,  5, TOUCH_ROLE_NOTE,   14, 67, TOUCH_CONTROL_NONE, "G4"  },
    { I2C_ADDR_TOUCH_2,  4, TOUCH_ROLE_NOTE,   15, 68, TOUCH_CONTROL_NONE, "G#4" },
    { I2C_ADDR_TOUCH_2,  3, TOUCH_ROLE_NOTE,   16, 69, TOUCH_CONTROL_NONE, "A4"  },
    { I2C_ADDR_TOUCH_2,  2, TOUCH_ROLE_NOTE,   17, 70, TOUCH_CONTROL_NONE, "A#4" },
    { I2C_ADDR_TOUCH_2,  1, TOUCH_ROLE_NOTE,   18, 71, TOUCH_CONTROL_NONE, "B4"  },
    { I2C_ADDR_TOUCH_2,  0, TOUCH_ROLE_NOTE,   19, 72, TOUCH_CONTROL_NONE, "C5"  },

    // ==================================================
    // TOUCH CONTROLLER 2 (0x21)
    // Control buttons
    // ==================================================

    { I2C_ADDR_TOUCH_2,  8, TOUCH_ROLE_CONTROL, TOUCH_NOTE_NONE, 0, TOUCH_CONTROL_OCTAVE, "OCTAVE" },
    { I2C_ADDR_TOUCH_2,  7, TOUCH_ROLE_CONTROL, TOUCH_NOTE_NONE, 0, TOUCH_CONTROL_VOICE,  "VOICE"  },
    { I2C_ADDR_TOUCH_2, 10, TOUCH_ROLE_CONTROL, TOUCH_NOTE_NONE, 0, TOUCH_CONTROL_MODE,   "MODE"   },
    { I2C_ADDR_TOUCH_2,  9, TOUCH_ROLE_CONTROL, TOUCH_NOTE_NONE, 0, TOUCH_CONTROL_RECORD, "RECORD" }
};

const uint8_t TOUCH_MAP_COUNT =
    sizeof(TOUCH_MAP) / sizeof(TOUCH_MAP[0]);