#pragma once

// ======================================================
// LED MANAGER
//
// High-level controller for the 32 PCA9685-driven LEDs on the PCB.
// Manages LED state (on/off/blink/breathe/pulse) and runs animations.
//
// Physical LED layout (see led_map.cpp for channel assignments):
//   - 20 note LEDs (F3-C5, one per key, split across I2C_ADDR_LED_1 and I2C_ADDR_LED_2)
//   -  5 octave indicator LEDs (I2C_ADDR_LED_1)
//   -  4 voice binary LEDs showing current voice (0-15) in binary (I2C_ADDR_LED_2)
//   -  3 status LEDs: record, mode, MIDI L/R (I2C_ADDR_LED_2)
//
// Both PCA9685 chips are on I2C1 (PIN_I2C1_SDA / PIN_I2C1_SCL).
// PIN_LED_EN must be driven high before LEDs will illuminate (set in system_init).
//
// Startup wave: a visual left-to-right sweep run during boot (LED_STARTUP_WAVE_ENABLED).
// led_manager_task() must be called every loop iteration to service animations.
// ======================================================

#include <stdint.h>
#include "led_map.h"
#include "led_animation.h"

bool led_manager_initialized();

bool led_manager_init();
void led_manager_task();

void led_manager_set_led(
    LedRole role,
    uint8_t index,
    LedMode mode,
    uint16_t brightness,
    uint32_t period_ms
);

// Note LEDs follow key presses (up to 20 simultaneously).
// In USB 500mA mode these are suppressed to stay within current budget
// (all 20 note LEDs at 10 mA each = 200 mA on the 5V rail).
// In USB 100mA mode the 5V rail is off entirely — LEDs produce no light
// regardless of PCA9685 register state.
void led_manager_set_note(uint8_t note_index, bool on);

// Enable or disable note LED updates based on power mode.
// When disabled, led_manager_set_note() is a no-op (status LEDs unaffected).
void led_manager_set_note_leds_enabled(bool enabled);
void led_manager_set_octave(int octave_offset, bool extended_mode);

// Normal voice display: shows the voice slot number in binary using the 4
// voice LEDs (labelled 1, 2, 4, 8 on the PCB). LEDs are solid ON or OFF.
void led_manager_set_voice(uint8_t voice);

// Voice bank selection display: called while bank selection mode is active.
// All LEDs blink at a moderate rate to signal the mode clearly.
//   Bank 0: all 4 LEDs blink at reduced brightness (reversed — normally
//           all off; blinking signals "zero is selected in bank mode").
//   Banks 1–15: only the LEDs representing set bits in the bank number
//           blink at full brightness; unset-bit LEDs stay off.
// Call led_manager_set_voice() to restore normal display on mode exit.
void led_manager_set_voice_bank_select(uint8_t bank);

void led_manager_set_recording(bool active);
void led_manager_set_mode(bool midi_mode_active);
void led_manager_set_midi_right(bool right_side);

void led_manager_start_startup_wave();
bool led_manager_startup_wave_active();
