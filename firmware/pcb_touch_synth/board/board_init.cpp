#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"

#include "./config/board_config.h"
#include "board_init.h"

bool board_init() {

    printf("Initializing board hardware...\n");

    // ==================================================
    // I2C0
    // ==================================================

    printf("Initializing I2C0...\n");

    i2c_init(i2c0, 400000);

    gpio_set_function(PIN_I2C0_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_I2C0_SCL, GPIO_FUNC_I2C);

    //Using external pull up resistors.
    gpio_disable_pulls(PIN_I2C0_SDA);
    gpio_disable_pulls(PIN_I2C0_SCL);

    // ==================================================
    // I2C1
    // ==================================================

    printf("Initializing I2C1...\n");

    i2c_init(i2c1, 400000);

    gpio_set_function(PIN_I2C1_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_I2C1_SCL, GPIO_FUNC_I2C);

    //Using external pull up resistors.
    gpio_disable_pulls(PIN_I2C1_SDA);
    gpio_disable_pulls(PIN_I2C1_SCL);

    // ==================================================
    // SPI0
    // ==================================================

    printf("Initializing SPI0...\n");

    spi_init(spi0, 1000 * 1000);

    gpio_set_function(PIN_SPI0_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI0_TX, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI0_RX, GPIO_FUNC_SPI);

    gpio_init(PIN_SPI0_CS_SD);
    gpio_set_dir(PIN_SPI0_CS_SD, GPIO_OUT);
    gpio_put(PIN_SPI0_CS_SD, 1);

    // ==================================================
    // DOTSTAR ENABLE
    // ==================================================

    printf("Initializing DotStar enable pin...\n");

    gpio_init(PIN_DOTSTAR_ENABLE);
    gpio_set_dir(PIN_DOTSTAR_ENABLE, GPIO_OUT);

    gpio_put(PIN_DOTSTAR_ENABLE, 1);

    printf("Board initialization complete.\n");

    // ==================================================
    // LED ENABLE
    // ==================================================

    printf("Initializing LED enable pin...\n");

    gpio_init(PIN_LED_EN);
    gpio_set_dir(PIN_LED_EN, GPIO_OUT);
    gpio_put(PIN_LED_EN, 0);   // keep LEDs disabled at boot

    // ==================================================
    // TOUCH CONTROLLERS
    // ==================================================

    printf("Initializing touch controller pins...\n");

    // IRQ pins are inputs from touch controllers
    gpio_init(PIN_TOUCH1_IRQ);
    gpio_set_dir(PIN_TOUCH1_IRQ, GPIO_IN);

    gpio_init(PIN_TOUCH2_IRQ);
    gpio_set_dir(PIN_TOUCH2_IRQ, GPIO_IN);

    printf("Holding touch controllers in reset...\n");

    // Shared reset pin is output from RP2040
    gpio_init(PIN_TOUCH_RESET);
    gpio_set_dir(PIN_TOUCH_RESET, GPIO_OUT);
    gpio_put(PIN_TOUCH_RESET, 0);

    // ==================================================
    // AUDIO I2S OUTPUT
    // ==================================================

    printf("Initializing audio I2S pins...\n");

    // These will eventually be owned by PIO/I2S driver.
    // For now, leave them as safe low outputs.
    gpio_init(PIN_AUDIO_I2S_BCLK);
    gpio_set_dir(PIN_AUDIO_I2S_BCLK, GPIO_OUT);
    gpio_put(PIN_AUDIO_I2S_BCLK, 0);

    gpio_init(PIN_AUDIO_I2S_LRCLK);
    gpio_set_dir(PIN_AUDIO_I2S_LRCLK, GPIO_OUT);
    gpio_put(PIN_AUDIO_I2S_LRCLK, 0);

    gpio_init(PIN_AUDIO_I2S_DATA);
    gpio_set_dir(PIN_AUDIO_I2S_DATA, GPIO_OUT);
    gpio_put(PIN_AUDIO_I2S_DATA, 0);

    // ==================================================
    // AUDIO CONTROL
    // ==================================================

    printf("Initializing audio control pins...\n");

    // Audio amp reset
    gpio_init(PIN_AUDIO_RESET);
    gpio_set_dir(PIN_AUDIO_RESET, GPIO_OUT);
    gpio_put(PIN_AUDIO_RESET, 0);   // hold amp in reset at boot

    // Audio IRQ from amp
    gpio_init(PIN_AUDIO_IRQ);
    gpio_set_dir(PIN_AUDIO_IRQ, GPIO_IN);

    // Rotary encoder volume pins
    gpio_init(PIN_AUDIO_VOL_A);
    gpio_set_dir(PIN_AUDIO_VOL_A, GPIO_IN);

    gpio_init(PIN_AUDIO_VOL_B);
    gpio_set_dir(PIN_AUDIO_VOL_B, GPIO_IN);

    // ==================================================
    // MICROPHONE I2S INPUT
    // ==================================================

    printf("Initializing microphone I2S data pin...\n");

    // Eventually owned by PIO/I2S mic driver.
    // For now, input is safe.
    gpio_init(PIN_MIC_I2S_DATA);
    gpio_set_dir(PIN_MIC_I2S_DATA, GPIO_IN);


    // ==================================================
    // USB MUX
    // ==================================================
    printf("Initializing USB mux select pin...\n");

    gpio_init(PIN_USB_MUX_SEL);
    gpio_set_dir(PIN_USB_MUX_SEL, GPIO_OUT);
    gpio_put(PIN_USB_MUX_SEL, 0);   // default: USB data routed away from MCU

    // ==================================================
    // BATTERY CHARGER / POWER PATH
    // ==================================================

    printf("Initializing charger pins...\n");
    gpio_init(PIN_CHARGER_IRQ);
    gpio_set_dir(PIN_CHARGER_IRQ, GPIO_IN);

    printf("Initializing power pins...\n");
    // 5V boost / rail enable
    gpio_init(PIN_5V_EN);
    gpio_set_dir(PIN_5V_EN, GPIO_OUT);
    gpio_put(PIN_5V_EN, 0);   // keep 5V rail disabled at boot

    printf("Board initialization complete.\n");
    return true;
}