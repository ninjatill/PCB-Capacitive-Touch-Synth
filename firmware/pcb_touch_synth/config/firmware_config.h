#pragma once

#include <stdint.h>
#include "pico/stdlib.h"

// ======================================================
// DEBUG / DEVELOPMENT OPTIONS
// ======================================================

#define ENABLE_DEBUG_CONSOLE 1
#define ENABLE_DEBUG_COMMANDS 1
#define ENABLE_DEBUG_BOOT_LOGS 1

#define DEBUG_FORCE_HIGH_USB_CURRENT 0
#define DEBUG_ENABLE_EXTRA_BOOT_LOGS 1
#define DEBUG_SKIP_AUDIO_INIT 0
constexpr uint32_t DEBUG_USB_SERIAL_WAIT_MS = 3000; // Time to wait at startup before USB debug printing begins, to allow USB serial connection.

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

// Startup wave runs during initializatoin to show/test every MCU-controlled LED iw working. 
// It's a nice visual indicator that the firmware is running and has reached the LED initialization stage.
constexpr bool LED_STARTUP_WAVE_ENABLED = true;
constexpr uint32_t LED_STARTUP_WAVE_DURATION_MS = 1500;
constexpr float LED_STARTUP_WAVE_WIDTH_MM = 12.0f;

// ======================================================
// OCTAVE CONFIGURATION
// ======================================================

// Piano middle C is C4.
// This is the default octave assigned to the lowest keyboard C.
constexpr int DEFAULT_BASE_OCTAVE = 4;

// Which of the 5 octave LEDs represents DEFAULT_BASE_OCTAVE.
// 0 = far left, 2 = center, 4 = far right.
constexpr int DEFAULT_OCTAVE_LED_INDEX = 2;

// Number of octave indicator LEDs on the PCB.
// In normal mode these represent offsets -2..+2 from DEFAULT_BASE_OCTAVE.
// In extended mode they represent -4..+4; the end LEDs breathe when the
// selected octave is beyond the visible LED range.
constexpr int OCTAVE_LED_COUNT = 5;

// Absolute octave range available in extended mode.
constexpr int EXTENDED_MIN_BASE_OCTAVE = 0;
constexpr int EXTENDED_MAX_BASE_OCTAVE = 8;

// Hold duration (press-to-release) on the OCTAVE button that toggles
// extended octave mode on or off.  A second hold of the same duration
// exits extended mode.  Short presses always cycle the octave offset.
constexpr uint32_t OCTAVE_EXTENDED_HOLD_MS = 5000;


// ======================================================
// VOICE CONFIGURATION
// ======================================================

// Number of voice slots per bank.  The 4 binary voice LEDs (labelled 1, 2,
// 4, 8 on the PCB) display the active slot in binary — e.g. voice 5 lights
// the '1' and '4' LEDs.  16 slots fills the 4-bit binary range exactly.
constexpr uint8_t VOICE_COUNT = 16;

// Slot 15 is reserved for the user's own recording.
constexpr uint8_t VOICE_RECORD_PLAYBACK = 15;

// Maximum number of voice banks.  Bank 0 is always the built-in defaults
// and is available without an SD card.  Banks 1–15 are defined entirely in
// the SD card config file using the "bank <n>" directive.  The 4 binary voice
// LEDs (1, 2, 4, 8) display the bank number in binary during bank selection
// mode, giving a natural limit of 16 banks (0–15).
// 16 banks × 16 voices × ~150 bytes base struct = ~38 KB static allocation.
// Per-key overrides and per-note samples use std::vector heap allocation only
// for entries that are actually configured, so unused slots cost nothing.
// The RP2040 has 264 KB SRAM; with ~100 KB used by firmware/stack/buffers,
// roughly 125 KB of heap remains for voice data — enough for thousands of
// key overrides across all banks (each KeyOverride ≈ 32 bytes heap).
constexpr uint8_t MAX_VOICE_BANKS = 16;

// Hold duration (press-to-release) on the VOICE button that toggles voice
// bank selection mode on or off.  While active, short presses cycle through
// configured banks (wrapping back to 0 after the last configured bank).
// The voice LEDs blink the current bank number in binary during selection.
// A second hold of the same duration exits bank selection and resumes normal
// voice cycling within the newly chosen bank.
// Bank selection mode is not available when only bank 0 is configured.
constexpr uint32_t VOICE_BANK_HOLD_MS = 5000;


// ======================================================
// RECORD CONFIGURATION
// ======================================================

constexpr uint32_t RECORD_HOLD_MS = 3000;
constexpr uint32_t RECORD_ARM_BLINK_MS = 500;
constexpr uint8_t  RECORD_ARM_BLINK_COUNT = 2;

// Recording is currently buffered in SRAM and written to SD card after the
// recording stops. The RP2040 has 264 KB total SRAM; with firmware, stack,
// DMA buffers and the sample pool, roughly 128 KB is available for recording.
//   128 KB / (44100 Hz × 3 bytes) ≈ 0.97 seconds
// RECORD_MAX_SAMPLE_MS is set conservatively below that limit.
// TODO: extend by implementing double-buffer streaming to SD card during
// recording, which removes the SRAM ceiling entirely.
constexpr uint32_t RECORD_MIN_SAMPLE_MS = 500;
constexpr uint32_t RECORD_MAX_SAMPLE_MS = 900;


// ======================================================
// MODE / MIDI CONFIGURATION
// ======================================================

constexpr uint32_t MIDI_LR_TOGGLE_HOLD_MS   = 3000;
constexpr uint32_t MODE_SETTINGS_HOLD_MS    = 7000;

// Holding the MODE button for this duration triggers a USB MIDI enumeration
// attempt.  10 seconds ensures it is an intentional act, not an accidental
// hold.  The MODE LED blinks at the threshold (while still held) to confirm
// the action before the button is released.
constexpr uint32_t MIDI_MODE_ENABLE_HOLD_MS = 10000;