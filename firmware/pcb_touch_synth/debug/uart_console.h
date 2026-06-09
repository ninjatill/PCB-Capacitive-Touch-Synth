#pragma once

// ======================================================
// UART DEBUG CONSOLE
//
// The UART debug path (GPIO 12 TX, GPIO 13 RX) is disabled by default.
// The traces are approximately 9 cm long — long enough to act as an
// antenna if the TX pin is driven continuously.  By keeping the pins
// as high-impedance inputs until UART is explicitly requested, the
// driven signal only appears on the trace when the console is active.
//
// WHEN TO ENABLE
//   Automatically: midi_manager_enable() calls uart_console_enable()
//     so the debug console survives the USB-to-MIDI transition.
//   Manually: debug command  "uart enable"
//             debug command  "uart disable"
//
// HARDWARE OPTIONS FOR PC/RP400 ACCESS
//   Option A — Particle Debugger or USB-UART adapter (CP2102, CH340):
//     Connect adapter TX → GPIO 13 (RX)
//     Connect adapter RX → GPIO 12 (TX)
//     Connect GND → GND
//     Open COM port at 115200 baud 8N1.
//
//   Option B — Particle Photon / Argon as UART bridge:
//     Flash a UART passthrough sketch to the Particle device.
//     Connect Argon D1(RX) → GPIO 12 (TX)
//     Connect Argon D0(TX) → GPIO 13 (RX)
//     Connect GND → GND
//     Open the Argon's USB serial port at 115200.
//
//   Option C — Raspberry Pi 400 GPIO UART directly:
//     Connect RP400 GPIO 15 (RXD) → GPIO 12 (TX)
//     Connect RP400 GPIO 14 (TXD) → GPIO 13 (RX)
//     Connect GND → GND
//     screen /dev/ttyAMA0 115200
//
// DEFAULT STATE
//   GPIO 12 and GPIO 13 are initialised as inputs with pull-ups.
//   The UART peripheral is not claimed.
//   No signal is driven onto the traces.
// ======================================================

#include <stdbool.h>

// Initialise GPIO 12/13 as high-impedance inputs with pull-ups.
// Called from board_init().  Does NOT claim the UART peripheral.
void uart_console_init();

// Claim the UART peripheral, configure GPIO 12/13 as UART TX/RX,
// and redirect all printf() output to UART in addition to USB CDC.
// Safe to call multiple times (idempotent).
void uart_console_enable();

// Release the UART peripheral and return GPIO 12/13 to high-impedance
// inputs.  printf() continues to work via USB CDC.
void uart_console_disable();

// True when the UART console is currently active.
bool uart_console_is_active();
