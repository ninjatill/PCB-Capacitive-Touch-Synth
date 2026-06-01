#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "audio_commands.h"
#include "audio_core.h"
#include "i2s_manager.h"
#include "../config/board_config.h"

static volatile AudioCoreMode current_mode = AUDIO_CORE_MODE_IDLE;
static volatile bool core1_running = false;

// forward declarations
static void audio_core_handle_command(const AudioCommand& command);

static void audio_core_configure_i2s_pins()
{
    printf("AUDIO CORE: configuring shared I2S pins...\n");

    // For now, leave as GPIO until PIO/DMA I2S is added.
    // Later:
    // BCLK  -> PIO output
    // LRCLK -> PIO output
    // DATA  -> PIO TX to TLV DAC
    // MIC   -> PIO RX from MEMS microphone

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
}

static void audio_core1_main()
{
    printf("AUDIO CORE: core 1 started.\n");

    core1_running = true;

    while (true) {
        AudioCommand command;

        while (audio_commands_pop(&command)) {
            audio_core_handle_command(command);
        }

        switch (current_mode) {
            case AUDIO_CORE_MODE_IDLE:
                tight_loop_contents();
                break;

            case AUDIO_CORE_MODE_PLAYBACK:
                // Future:
                // - synth active voices
                // - fill I2S TX buffer
                // - keep mic RX ignored
                tight_loop_contents();
                break;

            case AUDIO_CORE_MODE_RECORDING:
                i2s_manager_write_silence();
                tight_loop_contents();
                break;
        }
    }
}

bool audio_core_init()
{
    printf("AUDIO CORE: init...\n");

    audio_core_configure_i2s_pins();

    current_mode = AUDIO_CORE_MODE_IDLE;

    i2s_manager_init();

    printf("AUDIO CORE: initialized.\n");

    return true;
}

void audio_core_start_on_core1()
{
    printf("AUDIO CORE: launching core 1...\n");
    multicore_launch_core1(audio_core1_main);
}

static void audio_core_handle_command(const AudioCommand& command)
{
    switch (command.type) {
        case AUDIO_CMD_NOTE_ON:
            printf("AUDIO CORE: command NOTE_ON index=%u midi=%u voice=%u\n",
                   command.note_index,
                   command.midi_note,
                   command.voice);

            audio_core_set_mode(AUDIO_CORE_MODE_PLAYBACK);
            break;

        case AUDIO_CMD_NOTE_OFF:
            printf("AUDIO CORE: command NOTE_OFF index=%u midi=%u voice=%u\n",
                   command.note_index,
                   command.midi_note,
                   command.voice);

            // Later: release voice from synth engine.
            break;

        case AUDIO_CMD_START_RECORDING:
            printf("AUDIO CORE: command START_RECORDING\n");
            audio_core_set_mode(AUDIO_CORE_MODE_RECORDING);
            break;

        case AUDIO_CMD_STOP_RECORDING:
            printf("AUDIO CORE: command STOP_RECORDING\n");
            audio_core_set_mode(AUDIO_CORE_MODE_IDLE);
            break;

        case AUDIO_CMD_SET_IDLE:
            printf("AUDIO CORE: command SET_IDLE\n");
            audio_core_set_mode(AUDIO_CORE_MODE_IDLE);
            break;

        default:
            break;
    }
}

void audio_core_set_mode(AudioCoreMode mode)
{
    if (current_mode == mode) {
        return;
    }

    current_mode = mode;

    printf("AUDIO CORE: mode=%d\n", mode);

    switch (mode) {
        case AUDIO_CORE_MODE_IDLE:
            i2s_manager_stop();
            break;

        case AUDIO_CORE_MODE_PLAYBACK:
            i2s_manager_start_playback();
            break;

        case AUDIO_CORE_MODE_RECORDING:
            i2s_manager_start_recording();
            break;
    }
}

void audio_core_note_on(uint8_t note_index, uint8_t midi_note, uint8_t voice)
{
    printf("AUDIO CORE: note on index=%u midi=%u voice=%u\n",
           note_index,
           midi_note,
           voice);

    audio_core_set_mode(AUDIO_CORE_MODE_PLAYBACK);
}

void audio_core_note_off(uint8_t note_index, uint8_t midi_note)
{
    printf("AUDIO CORE: note off index=%u midi=%u\n",
           note_index,
           midi_note);
}