#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/interp.h"
#include "hardware/timer.h"
#include "hardware/watchdog.h"
#include "hardware/clocks.h"

#include "board/board_init.h"
#include "board/board_power.h"

#include "system/status_led.h"

uint32_t counter = 0;

void app_startup();
void app_loop();

int main()
{
    stdio_init_all();

    status_led_init();               // sets BOOTING

    // Give the USB serial port time to enumerate.
    sleep_ms(3000);

    printf("\n\n");
    printf("=====================================\n");
    printf(" PCB Piano RP2040 firmware starting\n");
    printf("=====================================\n");
    printf("USB serial initialized\n");
    printf("If you can read this, the RP2040 is alive.\n");

    //app_startup();    //Enable when we know board is alive.

    while (true) 
    {
        app_loop();
    }
}

void app_startup()
{
    printf("Starting PCB Piano...\n");

    board_init();

    board_power_safe_boot();

    // Later, when ready:
    // board_power_set_usb_mode(USB_POWER_500MA);
    // board_power_enable_5v();
    // board_power_enable_leds();
    // board_power_release_touch();
    // board_power_release_audio();

    // system_init();
}

void app_loop()
{
    printf("heartbeat\n");
    sleep_ms(1000);

    // Later:
    // system_tasks();
}
