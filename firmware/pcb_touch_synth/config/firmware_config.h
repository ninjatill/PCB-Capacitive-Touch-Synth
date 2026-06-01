#pragma once

#include <stdint.h>
#include "pico/stdlib.h"

// ======================================================
// DEBUG / DEVELOPMENT OPTIONS
// ======================================================

#define DEBUG_FORCE_HIGH_USB_CURRENT 0
#define DEBUG_ENABLE_EXTRA_BOOT_LOGS 1
#define DEBUG_SKIP_AUDIO_INIT 0

// ======================================================
// DOTSTAR STATUS LED CONFIGURATION
// ======================================================

constexpr bool STATUS_FRONT_LED_MIRRORS_DEV_LED = false;

constexpr int STATUS_BREATHE_PERIOD_MS = 2000;
constexpr int STATUS_FAULT_BLINK_MS = 250;
constexpr int STATUS_WARNING_BLINK_MS = 750;

// ======================================================
// LED CONFIGURATION
// ======================================================

constexpr uint8_t DOTSTAR_GLOBAL_BRIGHTNESS = 12;   // 0-31
constexpr uint16_t LED_GLOBAL_BRIGHTNESS = 2048;    // 0-4095 for PCA9685 LEDs
constexpr float LED_PWM_FREQUENCY_HZ = 1000.0f;

// ======================================================
// OCTAVE CONFIGURATION
// ======================================================

// Piano middle C is C4.
// This is the default octave assigned to the lowest keyboard C.
constexpr int DEFAULT_BASE_OCTAVE = 4;

// Which of the 5 octave LEDs represents DEFAULT_BASE_OCTAVE.
// 0 = far left, 2 = center, 4 = far right.
constexpr int DEFAULT_OCTAVE_LED_INDEX = 2;

// Main LED-indicated octave range is:
// DEFAULT_BASE_OCTAVE + (led_index - DEFAULT_OCTAVE_LED_INDEX)
constexpr int OCTAVE_LED_COUNT = 5;

// Full piano-ish extended octave range.
// We can tune this later.
constexpr int EXTENDED_MIN_BASE_OCTAVE = 0;
constexpr int EXTENDED_MAX_BASE_OCTAVE = 8;

// Long press to enable extended octave cycling.
constexpr uint32_t OCTAVE_EXTENDED_HOLD_MS = 5000;


// ======================================================
// VOICE CONFIGURATION
// ======================================================

constexpr uint8_t VOICE_COUNT = 16;
constexpr uint8_t VOICE_RECORD_PLAYBACK = 15;


// ======================================================
// RECORD CONFIGURATION
// ======================================================

constexpr uint32_t RECORD_HOLD_MS = 3000;
constexpr uint32_t RECORD_ARM_BLINK_MS = 500;
constexpr uint8_t  RECORD_ARM_BLINK_COUNT = 2;

constexpr uint32_t RECORD_MIN_SAMPLE_MS = 2000;
constexpr uint32_t RECORD_MAX_SAMPLE_MS = 5000;


// ======================================================
// MODE / MIDI CONFIGURATION
// ======================================================

constexpr uint32_t MIDI_LR_TOGGLE_HOLD_MS = 3000;
constexpr uint32_t MODE_SETTINGS_HOLD_MS = 7000;