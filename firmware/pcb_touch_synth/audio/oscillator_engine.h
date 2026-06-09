#pragma once

// ======================================================
// OSCILLATOR ENGINE — REAL-TIME WAVEFORM SYNTHESIS
//
// Implements a phase accumulator model for four waveform types.
// Each oscillator instance is stateless between calls except for the
// accumulated phase, so multiple voices can run independently.
//
// PHASE ACCUMULATOR
//   phase runs 0.0 → 1.0 over one cycle at the note's frequency.
//   Each call to oscillator_generate_sample() advances:
//     phase += frequency_hz / sample_rate_hz
//   and wraps at 1.0 to stay in range.
//
// OUTPUT RANGE
//   All waveforms output normalised floating-point in −1.0 to +1.0.
//   The synth engine multiplies by the ADSR envelope before mixing.
//
// USAGE (per voice, called by synth_engine on core 1)
//   oscillator_note_on()       — set waveform type, frequency, reset phase.
//   oscillator_generate_sample() — called once per output sample.
//   oscillator_note_off()      — mark inactive; generate_sample returns 0.
//
// NOTE: square, saw, and triangle are bandlimited only by the sample rate
// (no anti-aliasing). Aliasing artefacts will be audible at high notes.
// Wavetable voices (future) can replace the oscillator for those tones.
// ======================================================

#include <stdint.h>

#include "voice_definitions.h"

// Per-voice oscillator state. One instance per active note slot.
struct OscillatorState {
    OscillatorType type;
    float    frequency_hz;
    float    phase;   // normalised 0.0–1.0; advances each sample (unused for OSC_NOISE)
    uint32_t lfsr;    // xorshift32 PRNG state for OSC_NOISE; unused by other types
    bool     active;
};

bool oscillator_engine_init();

void oscillator_note_on(
    OscillatorState* osc,
    OscillatorType type,
    float frequency_hz
);

void oscillator_note_off(
    OscillatorState* osc
);

float oscillator_generate_sample(
    OscillatorState* osc,
    float sample_rate_hz
);