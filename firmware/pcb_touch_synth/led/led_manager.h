#pragma once

#include <stdint.h>
#include "led_map.h"
#include "led_animation.h"

bool led_manager_init();
void led_manager_task();

void led_manager_set_led(
    LedRole role,
    uint8_t index,
    LedMode mode,
    uint16_t brightness,
    uint32_t period_ms
);

void led_manager_set_note(uint8_t note_index, bool on);
void led_manager_set_octave(int octave_offset, bool extended_mode);
void led_manager_set_voice(uint8_t voice);

void led_manager_set_recording(bool active);
void led_manager_set_mode(bool midi_mode_active);
void led_manager_set_midi_right(bool right_side);