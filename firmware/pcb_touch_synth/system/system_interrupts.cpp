#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "../config/board_config.h"
#include "system_interrupts.h"

static volatile bool touch1_irq_pending = false;
static volatile bool touch2_irq_pending = false;
static volatile bool charger_irq_pending = false;
static volatile bool audio_irq_pending = false;

static void gpio_irq_callback(uint gpio, uint32_t events)
{
    if (gpio == PIN_TOUCH1_IRQ) {
        touch1_irq_pending = true;
    }
    else if (gpio == PIN_TOUCH2_IRQ) {
        touch2_irq_pending = true;
    }
    else if (gpio == PIN_CHARGER_IRQ) {
        charger_irq_pending = true;
    }
    else if (gpio == PIN_AUDIO_IRQ) {
        audio_irq_pending = true;
    }
}

void system_interrupts_init()
{
    printf("Configuring GPIO interrupts...\n");

    gpio_set_irq_enabled_with_callback(
        PIN_TOUCH1_IRQ,
        GPIO_IRQ_EDGE_FALL,
        true,
        &gpio_irq_callback
    );

    gpio_set_irq_enabled(PIN_TOUCH2_IRQ, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(PIN_CHARGER_IRQ, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(PIN_AUDIO_IRQ, GPIO_IRQ_EDGE_FALL, true);

    printf("GPIO interrupts configured.\n");
}

bool system_interrupt_touch1_pending() { return touch1_irq_pending; }
bool system_interrupt_touch2_pending() { return touch2_irq_pending; }
bool system_interrupt_charger_pending() { return charger_irq_pending; }
bool system_interrupt_audio_pending() { return audio_irq_pending; }

void system_interrupt_clear_touch1() { touch1_irq_pending = false; }
void system_interrupt_clear_touch2() { touch2_irq_pending = false; }
void system_interrupt_clear_charger() { charger_irq_pending = false; }
void system_interrupt_clear_audio() { audio_irq_pending = false; }