#pragma once

#include <stdint.h>

#include "../led/led_manager.h"
#include "../touch/touch_map.h"

bool control_manager_init();

void control_manager_note_pressed(uint8_t note_index, uint8_t midi_note);
void control_manager_note_released(uint8_t note_index, uint8_t midi_note);

void control_manager_control_pressed(TouchControl control);
void control_manager_control_released(TouchControl control);

void control_manager_task();

// Returns the current octave offset (-4..+4).
// Used by midi_manager to map incoming MIDI notes to key LED indices.
int control_manager_get_octave_offset();