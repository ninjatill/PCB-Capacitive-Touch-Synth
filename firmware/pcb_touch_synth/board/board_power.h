#pragma once

// ======================================================
// USB POWER MODES
//
// The MP2724 negotiates input current autonomously via CC/DPDM detection.
// The MCU reads the detected level via mp2724_get_power_source() and uses
// these values to make board-level enable/disable decisions (5V rail, audio).
// The MCU never overrides the MP2724's own IIN register — the chip owns that.
// ======================================================

enum UsbInputCurrentLimit
{
    USB_INPUT_100MA,    // Pre-enumeration or strict 100mA host → 5V OFF, no LEDs
    USB_INPUT_500MA,    // Standard Downstream Port (PC USB) → 5V OFF, no speaker
    USB_INPUT_1500MA,   // USB-C 1.5A or CDP → 5V ON, full features
    USB_INPUT_3000MA    // USB-C 3A or DCP adapter → 5V ON, full features
};


// ======================================================
// POWER / RAIL CONTROL
// ======================================================

void board_power_safe_boot();
void board_power_enable_5v();
void board_power_disable_5v();
void board_power_enable_leds();
void board_power_release_touch();
void board_power_release_audio();
void board_power_set_usb_input_limit(UsbInputCurrentLimit limit);
