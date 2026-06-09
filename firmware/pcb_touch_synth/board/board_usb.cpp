#include <stdio.h>

#include "pico/stdlib.h"
#include "tusb.h"

#include "../config/board_config.h"

void board_power_connect_usb_data()
{
    printf("Connecting USB data lines to RP2040...\n");
    gpio_put(PIN_USB_MUX_SEL, 1);
}

void board_power_disconnect_usb_data()
{
    printf("Disconnecting USB data lines from RP2040...\n");
    gpio_put(PIN_USB_MUX_SEL, 0);
}

bool board_usb_try_enumerate_midi(uint32_t timeout_ms)
{
    printf("USB MIDI: switching MUX to RP2040, waiting %u ms for host enumeration...\n",
           timeout_ms);

    board_power_connect_usb_data();

    // Poll tud_task() until the MIDI interface is mounted (host has enumerated
    // the device) or the timeout expires.
    uint32_t start = to_ms_since_boot(get_absolute_time());

    while (!tud_midi_mounted()) {
        tud_task();

        uint32_t elapsed = to_ms_since_boot(get_absolute_time()) - start;
        if (elapsed >= timeout_ms) {
            board_power_disconnect_usb_data();
            printf("USB MIDI: enumeration timeout after %u ms.\n", timeout_ms);
            return false;
        }
    }

    printf("USB MIDI: host enumerated MIDI device successfully.\n");
    return true;
}