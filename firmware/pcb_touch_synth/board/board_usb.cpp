#include <stdio.h>

#include "pico/stdlib.h"

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