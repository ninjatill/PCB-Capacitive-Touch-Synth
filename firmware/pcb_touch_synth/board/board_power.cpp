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

// Or disable 5V system
void board_power_disable_5v()
{
    printf("Disabling 5V rail...\n");

    gpio_put(PIN_5V_EN, 0);

    printf("5V rail disabled.\n");
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
    // The MP2724 sets its own IIN register autonomously after CC/DPDM detection.
    // The MCU does not write the IIN register — it only controls the 5V boost
    // rail based on how much current the detected source can provide.
    //
    // 5V rail powers: TLV320 Class-D speaker amp (SPKVDD) AND all 32 LEDs
    // (PCA9685 open-drain outputs sink LED current from 5V).
    // At 500mA or less, the 5V boost draw would consume most of the input
    // budget, so it must remain off.

    switch (limit) {
        case USB_INPUT_100MA:
            printf("Power: USB ~100mA — 5V boost disabled (no LEDs, no speaker).\n");
            gpio_put(PIN_5V_EN, 0);
            break;

        case USB_INPUT_500MA:
            printf("Power: USB 500mA — 5V boost disabled (no speaker, no note LEDs).\n");
            gpio_put(PIN_5V_EN, 0);
            break;

        case USB_INPUT_1500MA:
            printf("Power: USB 1.5A — enabling 5V boost (full features).\n");
            gpio_put(PIN_5V_EN, 1);
            sleep_ms(50);   // allow 5V to stabilise before consumers draw current
            break;

        case USB_INPUT_3000MA:
            printf("Power: USB 3A / adapter — enabling 5V boost (full features).\n");
            gpio_put(PIN_5V_EN, 1);
            sleep_ms(50);
            break;
    }
}
