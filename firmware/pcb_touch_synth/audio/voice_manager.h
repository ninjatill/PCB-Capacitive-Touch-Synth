#pragma once

// ======================================================
// VOICE MANAGER
//
// Tracks which of the 16 voice slots is currently selected and
// exposes the selection to the rest of the system.
//
// The selected voice index is displayed on the four binary LED
// indicators on the PCB (VOICE_LED_1/2/4/8 via led_manager).
// The LEDs show the 4-bit binary representation of the current
// voice number — e.g. voice 5 (0b0101) lights LED_1 and LED_4.
// led_manager_set_voice() handles the LED update; voice_manager
// only tracks the index.
//
// VOICE CYCLING
//   The VOICE touch button calls control_manager_control_pressed()
//   which calls voice_manager_next(), cycling 0→1→…→15→0.
//   voice_manager_set_current() allows direct selection (e.g. from
//   the debug console 'voice set <n>' command).
//
// VOICE_RECORD_PLAYBACK (slot 15)
//   Slot 15 is reserved for the user's own recording.
//   voice_manager_is_recorded_voice() returns true for this slot,
//   allowing callers to apply special behaviour (e.g. routing
//   playback through the ICS-43432 recording pipeline rather than
//   the synth engine).
//
// The voice definition data (waveform, ADSR, samples) lives in
// voice_definitions.h — voice_manager only holds the current index.
// ======================================================

#include <stdint.h>

bool voice_manager_initialized();

// Loads voice definitions from defaults and (future) SD card config.
bool voice_manager_init();

// Current voice slot index (0–15).
uint8_t voice_manager_get_current();

// Set the current voice directly. Clamps to 0–(MAX_VOICES−1).
void voice_manager_set_current(uint8_t voice);

// Advance to the next assigned voice slot in the current bank, skipping any
// unassigned slots.  Wraps at VOICE_COUNT and scans forward until it finds an
// assigned slot.  Returns the new slot index.
uint8_t voice_manager_next();

// ---- Voice bank management ----
// Bank 0 is always available (built-in defaults, no SD card required).
// Banks 1–(MAX_VOICE_BANKS−1) are populated from the SD card config file.
// Switching banks resets the active voice to the first assigned slot in the
// new bank, and notifies voice_definitions of the active bank change.

uint8_t voice_manager_get_bank();

// Set the active bank directly.  Clamps to the number of configured banks.
// Resets the active voice to the first assigned slot in the new bank.
void voice_manager_set_bank(uint8_t bank);

// Advance to the next configured bank, wrapping back to 0 after the last
// configured bank.  Does nothing if only bank 0 is configured.
// Returns the new bank index.
uint8_t voice_manager_next_bank();

// Returns true if the given slot is VOICE_RECORD_PLAYBACK (slot 15).
bool voice_manager_is_recorded_voice(uint8_t voice);