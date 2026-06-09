#pragma once

// ======================================================
// BOARD INITIALIZATION
//
// Configures all hardware-level peripherals and GPIO pins
// for the PCB Piano RP2040 board. Must be called first in
// system_init() before any driver or manager is initialized.
//
// Performs:
//   - I2C0 init (PIN_I2C0_SDA / PIN_I2C0_SCL) for touch controllers
//   - I2C1 init (PIN_I2C1_SDA / PIN_I2C1_SCL) for audio, power, LEDs
//   - SPI0 init (PIN_SPI0_* / PIN_DOTSTAR_*) shared by SD card and DotStar LEDs
//   - GPIO init for all control, status, and enable pins
//   - Holds PIN_TOUCH_RESET and PIN_AUDIO_RESET low (subsystems in reset)
//   - Keeps PIN_5V_EN and PIN_LED_EN low (rails off at boot)
//
// External pull-up resistors are fitted on I2C0 and I2C1;
// internal RP2040 pull-ups are disabled for both buses.
// ======================================================

bool board_init();