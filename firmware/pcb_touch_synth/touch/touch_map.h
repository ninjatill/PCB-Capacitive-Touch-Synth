#pragma once

#include <stdint.h>

enum TouchRole {
    TOUCH_ROLE_NOTE,
    TOUCH_ROLE_CONTROL
};

enum TouchControl {
    TOUCH_CONTROL_NONE,
    TOUCH_CONTROL_OCTAVE,
    TOUCH_CONTROL_VOICE,
    TOUCH_CONTROL_MODE,
    TOUCH_CONTROL_RECORD
};

struct TouchPadMap {
    uint8_t controller_addr;
    uint8_t channel;

    TouchRole role;

    uint8_t note_index;      // physical note order, 0-19
    uint8_t midi_note;       // MIDI note number, e.g. C4 = 60

    TouchControl control;

    const char* name;
};

constexpr uint8_t TOUCH_NOTE_NONE = 255;

extern const TouchPadMap TOUCH_MAP[];
extern const uint8_t TOUCH_MAP_COUNT;