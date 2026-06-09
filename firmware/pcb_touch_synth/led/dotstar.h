#pragma once

// ======================================================
// DOTSTAR (APA102) STATUS LED DRIVER
//
// Controls the 2 DotStar RGB LEDs used for MCU status indication:
//   LED 0 (DEV_LED):   on-board developer-facing LED
//   LED 1 (FRONT_LED): user-facing front-panel LED
//
// SPI BUS SHARING — IMPORTANT:
//   DotStar LEDs and the SD card share SPI0.
//   The 74AHCT2G125 dual bus buffer (active-low /OE = PIN_DOTSTAR_ENABLE)
//   isolates the DotStar from the bus when SD card operations are in progress.
//
//   Any code that performs SD card SPI transactions MUST:
//     1. Call dotstar_spi_acquire() to tri-state the level shifter output
//     2. Perform SD card operations
//     3. Call dotstar_spi_release() to re-enable the DotStar output
//
//   Failure to acquire the bus before SD ops will cause SPI data corruption
//   on the DotStar and may disrupt SD card communication.
//
// Brightness is clamped to DOTSTAR_GLOBAL_BRIGHTNESS (0-31) from firmware_config.h.
// ======================================================

#include "pico/stdlib.h"
#include <cstdint>

void dotstar_init();

// Disable the 74AHCT2G125 level shifter output before SD card SPI operations.
// Drives PIN_DOTSTAR_ENABLE high (tri-states the buffer output).
void dotstar_spi_acquire();

// Re-enable the 74AHCT2G125 level shifter after SD card SPI operations complete.
// Drives PIN_DOTSTAR_ENABLE low (enables the buffer output).
void dotstar_spi_release();

void dotstar_set_rgb(
    uint8_t led,
    uint8_t r,
    uint8_t g,
    uint8_t b
);

// Write buffered LED colours to the SPI bus.
// If dotstar_block_spi(true) has been called the write is silently skipped —
// the colour buffer is updated but no SPI transaction occurs.  The next
// dotstar_show() call after dotstar_block_spi(false) sends the current state.
void dotstar_show();

// Signal that another core/subsystem owns SPI0.
// When blocked, dotstar_show() skips SPI writes so state is never lost —
// the buffered colours are preserved and flushed as soon as unblocked.
// Call with true before SD card operations from core 1; false when done.
void dotstar_block_spi(bool blocked);