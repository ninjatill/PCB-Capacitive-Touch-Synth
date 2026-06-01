#include <stdio.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"

#include "rotary_encoder.h"
#include "../config/board_config.h"

static volatile int32_t encoder_delta = 0;
static volatile uint8_t last_encoder_state = 0;

static uint8_t read_encoder_state()
{
    uint8_t a = gpio_get(PIN_AUDIO_VOL_A) ? 1 : 0;
    uint8_t b = gpio_get(PIN_AUDIO_VOL_B) ? 1 : 0;

    return (a << 1) | b;
}

static void encoder_gpio_callback(uint gpio, uint32_t events)
{
    (void)gpio;
    (void)events;

    uint8_t current_state = read_encoder_state();

    uint8_t transition = (last_encoder_state << 2) | current_state;

    // Quadrature state table.
    // These are valid one-step transitions.
    switch (transition) {
        case 0b0001:
        case 0b0111:
        case 0b1110:
        case 0b1000:
            encoder_delta++;
            break;

        case 0b0010:
        case 0b1011:
        case 0b1101:
        case 0b0100:
            encoder_delta--;
            break;

        default:
            // No movement or invalid bounce transition.
            break;
    }

    last_encoder_state = current_state;
}

bool rotary_encoder_init()
{
    printf("Initializing rotary encoder...\n");

    gpio_init(PIN_AUDIO_VOL_A);
    gpio_set_dir(PIN_AUDIO_VOL_A, GPIO_IN);
    gpio_disable_pulls(PIN_AUDIO_VOL_A);     // Using external pull-up resistor

    gpio_init(PIN_AUDIO_VOL_B);
    gpio_set_dir(PIN_AUDIO_VOL_B, GPIO_IN);
    gpio_disable_pulls(PIN_AUDIO_VOL_B);     // Using external pull-up resistor

    last_encoder_state = read_encoder_state();
    encoder_delta = 0;

    gpio_set_irq_enabled_with_callback(
        PIN_AUDIO_VOL_A,
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
        true,
        &encoder_gpio_callback
    );

    gpio_set_irq_enabled(
        PIN_AUDIO_VOL_B,
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
        true
    );

    printf("Rotary encoder initialized. Initial state=%u\n", last_encoder_state);

    return true;
}

int32_t rotary_encoder_get_delta()
{
    int32_t delta;

    uint32_t interrupts = save_and_disable_interrupts();
    delta = encoder_delta;
    encoder_delta = 0;
    restore_interrupts(interrupts);

    return delta;
}

void rotary_encoder_clear_delta()
{
    uint32_t interrupts = save_and_disable_interrupts();
    encoder_delta = 0;
    restore_interrupts(interrupts);
}
