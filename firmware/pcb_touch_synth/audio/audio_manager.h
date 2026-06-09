#pragma once

// ======================================================
// AUDIO MANAGER
//
// Coordinates the TLV320DAC3100 DAC, rotary encoder volume control,
// and headphone jack detection. Bridges control input events to the
// audio command queue consumed by the synth engine on core 1.
//
// Dependencies:
//   - TLV320DAC3100 on I2C1 (I2C_ADDR_AUDIO_DAC), PIN_AUDIO_RESET de-asserted
//   - Rotary encoder on PIN_AUDIO_VOL_A / PIN_AUDIO_VOL_B
//   - PIN_AUDIO_IRQ for headphone detect notification
//
// audio_manager_task() polls the rotary encoder delta and updates
// volume. It should be called every main loop iteration.
// ======================================================

#include <stdint.h>

bool audio_manager_initialized();
bool audio_manager_headphones_inserted();

bool audio_manager_init();
void audio_manager_task();

void audio_manager_set_volume(int volume);
int audio_manager_get_volume();

void audio_manager_volume_up();
void audio_manager_volume_down();

void audio_manager_note_on(uint8_t note_index, uint8_t midi_note);
void audio_manager_note_off(uint8_t note_index, uint8_t midi_note);

// Called by system_tasks() when the power mode changes.
// Controls whether the Class-D speaker amplifier (SPKVDD on 5V rail) is
// permitted to be active.  Speaker is disabled on USB 100mA and 500mA
// to stay within USB current budget.  Headphones continue to work in all
// modes (they run from AVDD = 3.3V, not from the 5V rail).
void audio_manager_apply_power_mode(bool speaker_allowed);