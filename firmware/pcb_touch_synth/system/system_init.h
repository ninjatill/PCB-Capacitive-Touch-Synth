#pragma once

// ======================================================
// SYSTEM INITIALIZATION
//
// Orchestrates the full startup sequence for all subsystems.
// Called once from main() after stdio and status LED are ready.
//
// Initialization order (order is significant):
//   1. board_init()                  — GPIO, I2C, SPI peripheral setup
//   2. power_startup()               — MP2724 detection, 5V rail control
//   3. board_power_enable_leds()     — assert PIN_LED_EN for PCA9685 output
//   4. led_manager_init()            — PCA9685 setup; startup wave begins
//   5. board_power_release_audio()   — de-assert PIN_AUDIO_RESET for TLV320DAC3100
//   6. audio_core_init()             — I2S PIO setup (core 1)
//   7. voice_manager_init()          — load default voice definitions
//   8. audio_manager_init()          — TLV320DAC3100 I2C init, volume/HP setup
//   9. control_manager_init()        — octave/voice/record state machine
//  10. board_power_release_touch()   — de-assert PIN_TOUCH_RESET for MTCH2120
//  11. touch_manager_init()          — MTCH2120 I2C init, device ID validation
//  12. system_interrupts_init()      — register GPIO IRQ callbacks
//  13. audio_core_start_on_core1()   — launch audio render loop on core 1
//
// Returns false and sets SYSTEM_STATUS_FAULT if any step fails.
// ======================================================

#include <stdio.h>
#include "pico/stdlib.h"

bool system_init();