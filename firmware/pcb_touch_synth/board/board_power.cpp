#include <stdio.h>

#include "pico/stdlib.h"

#include "../config/board_config.h"
#include "board_power.h"

// ======================================================
// SAFE BOOT STATE
// ======================================================

void board_power_safe_boot()
{
    printf("Applying safe boot power configuration...\n");

    // --------------------------------------------------
    // Keep high-current systems disabled
    // --------------------------------------------------

    printf("Keeping 5V rail disabled...\n");
    gpio_put(PIN_5V_EN, 0);

    printf("Keeping LEDs disabled...\n");
    gpio_put(PIN_LED_EN, 0);

    // --------------------------------------------------
    // Hold subsystems in reset
    // --------------------------------------------------

    printf("Holding touch controllers in reset...\n");
    gpio_put(PIN_TOUCH_RESET, 0);

    printf("Holding audio system in reset...\n");
    gpio_put(PIN_AUDIO_RESET, 0);

    printf("Setting USB MUX to MP2724...\n");
    gpio_put(PIN_USB_MUX_SEL, 0);

    printf("Safe boot configuration complete.\n");
}

// ======================================================
// ENABLE 5V SYSTEM
// ======================================================

void board_power_enable_5v()
{
    printf("Enabling 5V rail...\n");

    gpio_put(PIN_5V_EN, 1);

    printf("Waiting for 5V rail to stabilize...\n");
    sleep_ms(500);

    printf("5V rail stabilization delay complete.\n");
}

// ======================================================
// ENABLE LED SYSTEM
// ======================================================

void board_power_enable_leds()
{
    printf("Enabling LEDs...\n");

    gpio_put(PIN_LED_EN, 1);
}

// ======================================================
// RELEASE TOUCH CONTROLLERS
// ======================================================

void board_power_release_touch()
{
    printf("Releasing touch controllers from reset...\n");

    gpio_put(PIN_TOUCH_RESET, 1);
}

// ======================================================
// RELEASE AUDIO SYSTEM
// ======================================================

void board_power_release_audio()
{
    printf("Releasing audio system from reset...\n");

    gpio_put(PIN_AUDIO_RESET, 1);
}

// ======================================================
// USB INPUT CURRENT LIMITS
// ======================================================

void board_power_set_usb_input_limit(UsbInputCurrentLimit limit)
{
    switch (limit)
    {
        case USB_INPUT_500MA:
            printf("USB input current limit target: 500mA\n");
            break;

        case USB_INPUT_1500MA:
            printf("USB input current limit target: 1.5A\n");
            break;

        case USB_INPUT_3000MA:
            printf("USB input current limit target: 3.0A\n");
            break;
    }

    printf("TODO: Write MP2724 register configuration here.\n");
}
