#pragma once

// ======================================================
// USB POWER MODES
// ======================================================

enum UsbInputCurrentLimit
{
    USB_INPUT_500MA,
    USB_INPUT_1500MA,
    USB_INPUT_3000MA
};


// ======================================================
// POWER / RAIL CONTROL
// ======================================================

void board_power_safe_boot();
void board_power_enable_5v();
void board_power_enable_leds();
void board_power_release_touch();
void board_power_release_audio();
void board_power_set_usb_input_limit(UsbInputCurrentLimit limit);
