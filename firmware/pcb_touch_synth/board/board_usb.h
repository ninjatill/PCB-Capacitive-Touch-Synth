#pragma once

#include <stdint.h>
#include <stdbool.h>

// ======================================================
// BOARD USB
//
// Controls the USB MUX (PIN_USB_MUX_SEL):
//   0 → MP2724  (CC/DPDM charge detection, default)
//   1 → RP2040  (USB device: CDC serial or MIDI)
//
// board_usb_try_enumerate_midi()
//   Switches the MUX to the RP2040 and waits up to timeout_ms for a USB
//   host to enumerate the device as a MIDI controller.
//   Returns true on success; restores MUX to MP2724 on failure.
//   Phase 2: replace sleep stub with TinyUSB polling
//     (tud_task() loop until tud_midi_mounted() or timeout).
// ======================================================

void board_power_connect_usb_data();
void board_power_disconnect_usb_data();

bool board_usb_try_enumerate_midi(uint32_t timeout_ms);