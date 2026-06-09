#pragma once

// ======================================================
// AUDIO CORE — TWO-CORE ARCHITECTURE
//
// The RP2040 has two Cortex-M0+ cores. Audio is split
// across them to keep the render loop deterministic:
//
//   CORE 0 — System / UI (runs system_tasks() every loop)
//     - Touch input, LED output, power management
//     - TLV320DAC3100 I2C configuration (volume, output switching)
//     - Pushes AudioCommand items to the inter-core command queue
//
//   CORE 1 — Audio render loop (runs audio_core1_main() forever)
//     - Owns the I2S PIO state machines and DMA channels
//     - Pops AudioCommands from the queue and dispatches them
//     - Calls synth_engine_task() each iteration to advance ADSR
//       and generate samples
//     - Fills the I2S TX DMA buffer with mixed output samples
//     - Drains the I2S RX DMA buffer (mic samples) into the
//       recording ring buffer
//
// CROSS-CORE COMMUNICATION
//   The only safe path from core 0 to core 1 is the AudioCommand
//   queue (audio_commands.h), which is a lock-free ring buffer.
//   Core 0 pushes; core 1 pops. Never call audio_core_note_on/off
//   directly from core 0 — push AUDIO_CMD_NOTE_ON/OFF instead.
//
//   The current_mode variable is volatile and written only by core 1
//   in response to commands. Core 0 may read it for status display
//   but must not write it directly.
//
// RENDER LOOP (once I2S + DMA are implemented)
//   Core 1 main loop expected flow per iteration:
//     1. Pop all pending AudioCommands and dispatch them.
//     2. Call synth_engine_task() to advance envelopes and oscillators.
//     3. If PLAYBACK: mix active voice samples into the I2S TX DMA buffer.
//        DMA double-buffers: while one half plays, core 1 fills the other.
//     4. If RECORDING: drain the I2S RX DMA buffer (ICS-43432 samples)
//        into the recording ring buffer for sample_loader to persist.
//     5. If IDLE: write silence to keep the I2S clock running (TLV320
//        requires continuous BCLK/WCLK or it enters standby).
//
// WHAT BELONGS IN audio_core.cpp
//   - audio_core1_main() render loop
//   - PIO I2S TX/RX setup (transfer from audio_core_configure_i2s_pins)
//   - DMA channel allocation and IRQ handler for buffer-complete events
//   - Sample mixing loop (calls synth_engine to get per-voice samples)
//   - Recording buffer drain (calls ics43432 ring buffer)
//   - Mode state machine (IDLE / PLAYBACK / RECORDING transitions)
//   - AudioCommand dispatch to synth_engine, i2s_manager, ics43432
//
// WHAT DOES NOT BELONG HERE
//   - Voice parameter definitions       → voice_definitions.cpp
//   - ADSR envelope math                → synth_engine.cpp
//   - Oscillator waveform generation    → oscillator_engine.cpp
//   - Sample file loading from SD card  → sample_loader.cpp
//   - TLV320 I2C register writes        → tlv320dac3100.cpp / audio_manager.cpp
//   - Touch/LED/control logic           → control_manager.cpp
// ======================================================

#include <stdint.h>

// Current operating mode of the audio render loop on core 1.
// Transitions are driven by AudioCommands from core 0.
enum AudioCoreMode {
    AUDIO_CORE_MODE_IDLE,       // No active notes or recording. I2S clock kept running (silence).
    AUDIO_CORE_MODE_PLAYBACK,   // One or more notes active. Core 1 synthesises and fills I2S TX buffer.
    AUDIO_CORE_MODE_RECORDING   // Capturing from ICS-43432. DAC output silenced by audio_manager.
};

// Called once from system_init() on core 0.
// Configures I2S GPIO pins and initialises the i2s_manager (PIO stub for now).
bool audio_core_init();

// Launches audio_core1_main() on core 1. Call at the end of system_init(),
// after all other subsystems are ready, so core 1 does not race on I2C or SPI.
void audio_core_start_on_core1();

// Switch the render loop mode. Called by core 1's command handler only —
// do NOT call from core 0 directly.
void audio_core_set_mode(AudioCoreMode mode);

// Dispatched by core 1 when it processes AUDIO_CMD_NOTE_ON / NOTE_OFF
// from the command queue. These call into synth_engine to track voice state.
// Do NOT call these directly from core 0 — push a command instead.
void audio_core_note_on(uint8_t note_index, uint8_t midi_note, uint8_t voice);
void audio_core_note_off(uint8_t note_index, uint8_t midi_note);