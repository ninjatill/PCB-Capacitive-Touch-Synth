#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "audio_commands.h"
#include "audio_core.h"
#include "i2s_manager.h"
#include "midi_manager.h"
#include "synth_engine.h"
#include "sysex_transfer.h"
#include "../config/board_config.h"

// ======================================================
// CROSS-CORE STATE
//
// Both variables are written only by core 1 and read by core 0
// for status display. 'volatile' prevents the compiler from
// caching stale values in registers across cores.
// ======================================================

static volatile AudioCoreMode current_mode = AUDIO_CORE_MODE_IDLE;
static volatile bool core1_running = false;

// Forward declaration — defined at the bottom of this file so the
// render loop at the top reads like a spec.
static void audio_core_handle_command(const AudioCommand& command);

// ======================================================
// I2S PIN SETUP
//
// CURRENT: pins held as safe-low GPIO outputs while the PIO
// programs are not yet implemented.
//
// FUTURE — when i2s_manager.cpp is implemented, replace this
// function with PIO state machine setup:
//
//   TX (output to TLV320DAC3100):
//     One PIO SM drives:
//       PIN_AUDIO_I2S_BCLK  — bit clock output (AUDIO_BCLK_HZ = 44100×64)
//       PIN_AUDIO_I2S_LRCLK — word select output (44100 Hz)
//       PIN_AUDIO_I2S_DATA  — 32-bit I2S serial data to DAC
//     DMA feeds 32-bit stereo samples from a double-buffer in SRAM.
//
//   RX (input from ICS-43432 MEMS mic):
//     A second PIO SM (or the same SM with an input pin) reads:
//       PIN_MIC_I2S_DATA    — 24-bit serial data from mic (right channel,
//                             LR pin tied high, data follows WS rising edge)
//     BCLK and WCLK are shared with the TX state machine — the mic
//     is a passive slave on the same clock lines.
//     DMA drains 32-bit words (24-bit data, left-justified) into a
//     recording ring buffer. Sign-extend via >> 8 on read.
//
//   Both DMA channels should use double-buffering (ping-pong):
//     - DMA raises an IRQ on each half-complete and full-complete event.
//     - The IRQ handler sets a flag; core 1's main loop checks the flag
//       and fills/drains the inactive half while the other half plays.
// ======================================================

static void audio_core_configure_i2s_pins()
{
    // PIO owns BCLK, LRCLK, and DATA after i2s_manager_init() loads the
    // i2s_tx program and calls pio_gpio_init() on each pin.  We only need
    // to configure the mic input here (it is not touched by the TX PIO SM).
    gpio_init(PIN_MIC_I2S_DATA);
    gpio_set_dir(PIN_MIC_I2S_DATA, GPIO_IN);

    printf("AUDIO CORE: I2S pins delegated to PIO (MIC GPIO %d set as input).\n",
           PIN_MIC_I2S_DATA);
}

// ======================================================
// CORE 1 RENDER LOOP
//
// This is the entry point launched by audio_core_start_on_core1().
// It runs forever and owns everything audio-related after launch.
//
// CURRENT: polls the command queue and stubs out the render work.
//
// FUTURE render loop structure (each iteration):
//
//   1. COMMAND DISPATCH
//      Drain the AudioCommand queue. Commands arrive from core 0 via
//      audio_manager (NOTE_ON, NOTE_OFF, START_RECORDING, etc.).
//
//   2. SYNTH ENGINE TICK  [PLAYBACK mode only]
//      Call synth_engine_task() to advance ADSR envelopes, advance
//      oscillator phase accumulators, and generate one frame's worth
//      of samples per active voice. Each voice writes into its own
//      scratch buffer.
//
//   3. MIX AND WRITE TO I2S TX DMA BUFFER  [PLAYBACK mode]
//      Sum all active voice buffers (with per-voice gain applied).
//      Clamp to 32-bit signed range.
//      Write into the inactive half of the I2S TX DMA double-buffer.
//      The inactive half is the one NOT currently being played by DMA.
//      Check the DMA half-complete/complete IRQ flag to know which half.
//
//   4. DRAIN I2S RX DMA BUFFER  [RECORDING mode]
//      Read the inactive half of the ICS-43432 RX DMA buffer.
//      Sign-extend 24-bit samples: int32_t s = (int32_t)raw >> 8;
//      Write into the recording ring buffer for sample_loader.
//      Write silence to the TX buffer so there is no speaker feedback.
//
//   5. IDLE
//      No render work. The I2S clock must stay running (the TLV320
//      enters standby if BCLK/WCLK stop). i2s_manager_write_silence()
//      keeps the DMA buffer filled with zeros.
//
// NOTE ON TIMING:
//   At 44100 Hz stereo, one DMA half-buffer of N stereo samples gives
//   (N / 44100) seconds of render budget. For N = 128 that is ~2.9 ms.
//   The synth engine tick + mix must complete within that window.
//   Keep the loop lean — no I2C, no printf, no blocking calls.
// ======================================================

// Mono 24-bit mix buffer — filled by synth_engine_fill_buffer() each iteration.
// Static so it is in BSS (zero-initialised) rather than the stack.
static int32_t mix_buffer[HALF_BUFFER_FRAMES];

static void audio_core1_main()
{
    printf("AUDIO CORE: core 1 started.\n");

    core1_running = true;

    while (true) {

        // ---- STEP 1: Drain the inter-core command queue ----
        // Commands arrive from core 0 via audio_manager (NOTE_ON, NOTE_OFF, etc.).
        // We drain here AND after the blocking DMA wait so latency stays low.
        AudioCommand command;
        while (audio_commands_pop(&command)) {
            audio_core_handle_command(command);
        }

        // ---- STEP 2: Wait for DMA to need a new buffer ----
        // If I2S is not yet running (no note has been played), skip the buffer
        // work and poll commands at CPU speed.  The DMA starts on the first
        // NOTE_ON via audio_core_note_on() → audio_core_set_mode(PLAYBACK)
        // → i2s_manager_start_playback().
        uint32_t* dma_buf = i2s_manager_get_next_write_buffer();

        if (dma_buf == nullptr) {
            // In MIDI mode: core 1 is free — run the SysEx file transfer task.
            // Reads SD card chunks and pushes encoded SysEx blocks to the
            // ring buffer; core 0 drains them to USB in system_tasks().
            if (midi_manager_is_enabled() && sysex_transfer_is_active()) {
                sysex_transfer_task();
            } else {
                tight_loop_contents();
            }
            continue;
        }

        // After the blocking wait, drain commands again — up to 2.9 ms of
        // commands may have queued while we were waiting for DMA.
        while (audio_commands_pop(&command)) {
            audio_core_handle_command(command);
        }

        // ---- STEP 3: Fill the inactive DMA buffer based on current mode ----
        switch (current_mode) {

            case AUDIO_CORE_MODE_PLAYBACK: {
                // Advance synth engine ADSR and oscillator state.
                synth_engine_task();

                // Generate HALF_BUFFER_FRAMES mono 24-bit samples.
                synth_engine_fill_buffer(mix_buffer, HALF_BUFFER_FRAMES);

                // Expand mono 24-bit → stereo 32-bit I2S words.
                // The TLV320DAC3100 is configured for 32-bit I2S words.
                // Our synth produces signed 24-bit samples in [-0x800000, 0x7FFFFF].
                // Left-shift by 8 to place audio in the upper 24 bits of the word;
                // the lower 8 bits are zero-padded (≡ multiplying by 256, which
                // the DAC ignores since it reads the upper 24 bits of the frame).
                for (uint32_t f = 0; f < HALF_BUFFER_FRAMES; f++) {
                    uint32_t word = (uint32_t)(int32_t)mix_buffer[f] << 8;
                    dma_buf[f * 2 + 0] = word;  // left  channel
                    dma_buf[f * 2 + 1] = word;  // right channel (mono duplicated)
                }

                // Transition to IDLE once all release tails decay to silence.
                if (!synth_engine_notes_active()) {
                    audio_core_set_mode(AUDIO_CORE_MODE_IDLE);
                }
                break;
            }

            case AUDIO_CORE_MODE_IDLE:
                // DAC must keep receiving a continuous BCLK/WCLK signal or it
                // enters standby.  i2s_manager_stop() is a no-op; instead we
                // feed zeros so the speaker stays silent but the clock runs.
                i2s_manager_write_silence();
                break;

            case AUDIO_CORE_MODE_RECORDING:
                // DAC is silenced during mic capture to prevent speaker feedback.
                // Phase 2: drain I2S RX DMA into recording ring buffer here.
                i2s_manager_write_silence();
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

// ======================================================
// COMMAND DISPATCH (runs on core 1)
//
// All commands originate from core 0 (audio_manager.cpp) and
// arrive here via the lock-free AudioCommand queue.
// This is the ONLY place commands are consumed — never call
// audio_core_note_on/off directly from core 0.
// ======================================================

static void audio_core_handle_command(const AudioCommand& command)
{
    switch (command.type) {

        case AUDIO_CMD_NOTE_ON:
            printf("AUDIO CORE: NOTE_ON index=%u midi=%u voice=%u\n",
                   command.note_index, command.midi_note, command.voice);
            // Notify the synth engine so it can allocate a voice slot
            // and start the ADSR attack phase.
            audio_core_note_on(command.note_index, command.midi_note, command.voice);
            break;

        case AUDIO_CMD_NOTE_OFF:
            printf("AUDIO CORE: NOTE_OFF index=%u midi=%u\n",
                   command.note_index, command.midi_note);
            // Trigger the ADSR release phase; voice is freed once
            // the envelope reaches silence.
            audio_core_note_off(command.note_index, command.midi_note);
            break;

        case AUDIO_CMD_START_RECORDING:
            printf("AUDIO CORE: START_RECORDING\n");
            // Switch render loop to drain mic RX buffer.
            // DAC is silenced by audio_manager before this command is sent.
            audio_core_set_mode(AUDIO_CORE_MODE_RECORDING);
            break;

        case AUDIO_CMD_STOP_RECORDING:
            printf("AUDIO CORE: STOP_RECORDING\n");
            audio_core_set_mode(AUDIO_CORE_MODE_IDLE);
            break;

        case AUDIO_CMD_SET_IDLE:
            printf("AUDIO CORE: SET_IDLE\n");
            audio_core_set_mode(AUDIO_CORE_MODE_IDLE);
            break;

        default:
            break;
    }
}

// ======================================================
// MODE STATE MACHINE (runs on core 1)
//
// Mode transitions start and stop the I2S DMA streams.
// All four i2s_manager calls are stubs until PIO is implemented.
//
// FUTURE transition behaviour:
//   IDLE      → nothing playing. DMA TX filled with zeros (silence).
//               DMA RX halted or discarded.
//   PLAYBACK  → DMA TX runs. Core 1 render loop fills it each half-buffer.
//               DMA RX halted (no recording).
//   RECORDING → DMA RX runs. Core 1 drains it into recording ring buffer.
//               DMA TX filled with zeros (silence) to prevent feedback.
//
// Note: the TLV320DAC3100 requires continuous BCLK/WCLK regardless of
// mode. i2s_manager should never stop the PIO clock — only the DMA data.
// ======================================================

void audio_core_set_mode(AudioCoreMode mode)
{
    if (current_mode == mode) {
        return;
    }

    static const char* mode_names[] = { "IDLE", "PLAYBACK", "RECORDING" };
    printf("AUDIO CORE: mode %s → %s\n",
           mode_names[current_mode], mode_names[mode]);

    current_mode = mode;

    switch (mode) {
        case AUDIO_CORE_MODE_IDLE:
            // Stop DMA data transfer; keep PIO clock running.
            i2s_manager_stop();
            break;

        case AUDIO_CORE_MODE_PLAYBACK:
            // Start (or resume) DMA TX. Core 1 will fill the buffer
            // each iteration via synth_engine_task() + mix.
            i2s_manager_start_playback();
            break;

        case AUDIO_CORE_MODE_RECORDING:
            // Start DMA RX capture from ICS-43432.
            // DAC is already muted by audio_manager before this command.
            i2s_manager_start_recording();
            break;
    }
}

// ======================================================
// NOTE EVENT HANDLERS (run on core 1, called from command dispatch)
//
// These are the boundary between the command queue and the synth engine.
// They must remain fast — no blocking, no I2C, no printf in the hot path
// once real audio is rendering.
// ======================================================

void audio_core_note_on(uint8_t note_index, uint8_t midi_note, uint8_t voice)
{
    // Tell the synth engine to allocate a slot and begin the attack phase.
    // synth_engine_note_on() tracks the note in active_notes[].
    synth_engine_note_on(note_index, midi_note, voice);

    // Switch to PLAYBACK so the render loop starts filling the I2S buffer.
    audio_core_set_mode(AUDIO_CORE_MODE_PLAYBACK);
}

void audio_core_note_off(uint8_t note_index, uint8_t midi_note)
{
    // Begin the ADSR release phase. The voice slot is not freed here —
    // synth_engine_task() will free it once the envelope reaches silence.
    synth_engine_note_off(note_index, midi_note);

    // TODO: if synth_engine_notes_active() returns false after release,
    // transition back to IDLE so the render loop stops burning CPU.
    // This needs a short grace period so the release tail can play out.
}