#pragma once

// ======================================================
// SYNTH ENGINE — PER-NOTE AUDIO GENERATION (runs on core 1)
//
// Manages up to MAX_ACTIVE_NOTES simultaneously playing notes. Each
// active note has its own ADSR envelope and voice-type-specific
// playback state (oscillator or SRAM sample buffer pointer).
//
// VOICE TYPE DISPATCH
//   OSCILLATOR  — Calls oscillator_engine to generate one sample per
//                 frame using the phase accumulator model.
//   PLAYBACK    — Reads from a preloaded SRAM buffer (loaded at boot
//                 by sample_loader). Position advances by pitch_ratio
//                 per frame to allow basic pitch shifting.
//   WAVETABLE   — Not yet implemented; falls back to silence.
//
// ADSR LIFECYCLE
//   note_on  → ATTACK  phase starts, level ramps 0→1.
//   key held → DECAY   level ramps to sustain_level, then SUSTAIN holds.
//   note_off → RELEASE level ramps to 0, then slot is freed.
//
// OUTPUT
//   synth_engine_fill_buffer() mixes all active notes into a mono
//   int32_t buffer of 24-bit signed samples. The audio_core render
//   loop calls this once per DMA half-buffer and feeds the result to
//   the I2S TX DMA (duplicating to stereo for the TLV320DAC3100).
// ======================================================

#include <stdint.h>

// ADSR amplitude envelope phase.
enum AdsrPhase {
    ADSR_IDLE,      // Slot is free.
    ADSR_ATTACK,    // Ramp 0 → 1.0 over attack_ms.
    ADSR_DECAY,     // Ramp 1.0 → sustain_level over decay_ms.
    ADSR_SUSTAIN,   // Hold at sustain_level while key is down.
    ADSR_RELEASE    // Ramp sustain_level → 0 over release_ms after key-up.
                    // Slot becomes IDLE automatically when level reaches 0.
};

bool synth_engine_init();

// Called by audio_core_note_on() on core 1 after a NOTE_ON command arrives.
// Sets up oscillator state or sample buffer pointer based on voice type.
void synth_engine_note_on(uint8_t note_index, uint8_t midi_note, uint8_t voice_index);

// Called by audio_core_note_off() on core 1. Transitions the note to
// ADSR_RELEASE — the slot is freed when the release tail reaches silence.
void synth_engine_note_off(uint8_t note_index, uint8_t midi_note);

// Returns true while any note is in a non-IDLE ADSR phase (including release).
// Used by audio_manager to decide whether the encoder chirp is appropriate.
bool synth_engine_notes_active();

// Generate 'num_frames' mono audio samples into 'buffer'.
// Each sample is a 24-bit signed int in [-0x800000, 0x7FFFFF].
// Called by the core 1 render loop to fill the I2S TX DMA half-buffer.
// Notes whose ADSR release completes mid-buffer are freed within this call.
void synth_engine_fill_buffer(int32_t* buffer, uint32_t num_frames);

// Advance internal state for one render iteration. Called at the top of
// the core 1 loop before fill_buffer() — handles any per-tick housekeeping.
void synth_engine_task();

void synth_engine_print_status();