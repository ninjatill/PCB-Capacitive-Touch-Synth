#include <stdio.h>

#include "pico/stdlib.h"

#include "i2s_manager.h"
#include "../config/board_config.h"

static bool i2s_initialized = false;
static bool i2s_running = false;
static bool i2s_recording = false;

bool i2s_manager_init()
{
    printf("I2S manager: init...\n");

    gpio_init(PIN_AUDIO_I2S_BCLK);
    gpio_init(PIN_AUDIO_I2S_LRCLK);
    gpio_init(PIN_AUDIO_I2S_DATA);
    gpio_init(PIN_MIC_I2S_DATA);

    gpio_set_dir(PIN_AUDIO_I2S_BCLK, GPIO_OUT);
    gpio_set_dir(PIN_AUDIO_I2S_LRCLK, GPIO_OUT);
    gpio_set_dir(PIN_AUDIO_I2S_DATA, GPIO_OUT);
    gpio_set_dir(PIN_MIC_I2S_DATA, GPIO_IN);

    gpio_put(PIN_AUDIO_I2S_BCLK, 0);
    gpio_put(PIN_AUDIO_I2S_LRCLK, 0);
    gpio_put(PIN_AUDIO_I2S_DATA, 0);

    i2s_initialized = true;
    i2s_running = false;
    i2s_recording = false;

    printf("I2S manager: initialized.\n");
    printf("  BCLK  GPIO %d\n", PIN_AUDIO_I2S_BCLK);
    printf("  LRCLK GPIO %d\n", PIN_AUDIO_I2S_LRCLK);
    printf("  DAC   GPIO %d\n", PIN_AUDIO_I2S_DATA);
    printf("  MIC   GPIO %d\n", PIN_MIC_I2S_DATA);

    return true;
}

void i2s_manager_start_playback()
{
    if (!i2s_initialized) {
        printf("I2S manager: cannot start playback, not initialized.\n");
        return;
    }

    if (i2s_running && !i2s_recording) {
        return;
    }

    printf("I2S manager: start playback.\n");

    i2s_running = true;
    i2s_recording = false;

    // Future:
    // - start PIO I2S TX
    // - start DMA feeding DAC samples
    // - optionally ignore mic RX
}

void i2s_manager_start_recording()
{
    if (!i2s_initialized) {
        printf("I2S manager: cannot start recording, not initialized.\n");
        return;
    }

    if (i2s_running && i2s_recording) {
        return;
    }

    printf("I2S manager: start recording.\n");

    i2s_running = true;
    i2s_recording = true;

    // Future:
    // - keep BCLK/LRCLK running
    // - feed DAC silence
    // - enable mic RX path
    // - route samples to recording buffer / SD pipeline
}

void i2s_manager_stop()
{
    if (!i2s_initialized) {
        return;
    }

    if (!i2s_running) {
        return;
    }

    printf("I2S manager: stop.\n");

    i2s_running = false;
    i2s_recording = false;

    gpio_put(PIN_AUDIO_I2S_BCLK, 0);
    gpio_put(PIN_AUDIO_I2S_LRCLK, 0);
    gpio_put(PIN_AUDIO_I2S_DATA, 0);

    // Future:
    // - stop DMA safely
    // - stop/restart PIO state machines
}

void i2s_manager_write_silence()
{
    if (!i2s_initialized || !i2s_running) {
        return;
    }

    // Future:
    // - push zero samples into TX buffer
    // - used during recording so DAC output stays silent
}