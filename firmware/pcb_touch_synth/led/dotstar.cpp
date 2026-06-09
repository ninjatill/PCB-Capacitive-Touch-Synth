#include <stdio.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/spi.h"

#include "dotstar.h"
#include "../config/board_config.h"
#include "../config/firmware_config.h"

static constexpr uint8_t NUM_DOTSTARS = 2;

static uint8_t led_buffer[NUM_DOTSTARS][3];

// When true, dotstar_show() skips SPI writes.  The led_buffer is still
// updated so state is never lost — the next unblocked show() sends current state.
static volatile bool _spi_blocked = false;

void dotstar_block_spi(bool blocked)
{
    _spi_blocked = blocked;
}

static uint8_t clamp_dotstar_brightness()
{
    if (DOTSTAR_GLOBAL_BRIGHTNESS > 31) {
        return 31;
    }

    return DOTSTAR_GLOBAL_BRIGHTNESS;
}

void dotstar_spi_acquire()
{
    // Tri-state the 74AHCT2G125 output so SD card SPI traffic
    // does not reach the DotStar data/clock lines.
    gpio_put(PIN_DOTSTAR_ENABLE, 1);
}

void dotstar_spi_release()
{
    // Re-enable the 74AHCT2G125 output after SD card operations complete.
    gpio_put(PIN_DOTSTAR_ENABLE, 0);
}

void dotstar_init()
{
    printf("Initializing DotStar status LEDs...\n");

    gpio_init(PIN_DOTSTAR_ENABLE);
    gpio_set_dir(PIN_DOTSTAR_ENABLE, GPIO_OUT);

    // 74AHCT2G125 /OE is active-low. Drive low to enable DotStar output.
    gpio_put(PIN_DOTSTAR_ENABLE, 0);

    // dotstar_init() is called from status_led_init() before board_init(), so it
    // initializes SPI0 here. board_init() calls spi_init(spi0,...) again later at
    // the same speed — the SDK re-init is safe and idempotent.
    spi_init(spi0, 1000000);

    gpio_set_function(PIN_DOTSTAR_DATA, GPIO_FUNC_SPI);
    gpio_set_function(PIN_DOTSTAR_CLK, GPIO_FUNC_SPI);

    for (uint8_t i = 0; i < NUM_DOTSTARS; i++) {
        led_buffer[i][0] = 0;
        led_buffer[i][1] = 0;
        led_buffer[i][2] = 0;
    }

    dotstar_show();

    printf("DotStar status LEDs initialized.\n");
}

void dotstar_set_rgb(uint8_t led, uint8_t r, uint8_t g, uint8_t b)
{
    if (led >= NUM_DOTSTARS) {
        printf("DotStar invalid LED index: %u\n", led);
        return;
    }

    led_buffer[led][0] = r;
    led_buffer[led][1] = g;
    led_buffer[led][2] = b;
}

void dotstar_show()
{
    // If another core owns SPI0, skip the hardware write.
    // led_buffer already holds the desired state; the next unblocked
    // call will send it, so no visual state is ever lost.
    if (_spi_blocked) return;

    uint8_t start_frame[4] = { 0x00, 0x00, 0x00, 0x00 };
    uint8_t end_frame[4]   = { 0xFF, 0xFF, 0xFF, 0xFF };

    spi_write_blocking(spi0, start_frame, sizeof(start_frame));

    uint8_t brightness = clamp_dotstar_brightness();

    for (uint8_t i = 0; i < NUM_DOTSTARS; i++) {
        uint8_t frame[4];

        frame[0] = 0xE0 | brightness;

        // DotStar color order is B, G, R.
        frame[1] = led_buffer[i][2]; // B
        frame[2] = led_buffer[i][1]; // G
        frame[3] = led_buffer[i][0]; // R

        spi_write_blocking(spi0, frame, sizeof(frame));
    }

    spi_write_blocking(spi0, end_frame, sizeof(end_frame));
}