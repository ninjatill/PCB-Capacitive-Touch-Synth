#include <math.h>
#include <stdio.h>

#include "audio_constants.h"
#include "oscillator_engine.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Running counter used to seed each noise oscillator with a unique starting
// state so simultaneous noise voices do not produce correlated (cancelling) output.
static uint32_t noise_seed_counter = 0xACE1AC35u;

bool oscillator_engine_init()
{
    printf("Oscillator engine initialized.\n");
    return true;
}

void oscillator_note_on(
    OscillatorState* osc,
    OscillatorType type,
    float frequency_hz
)
{
    if (osc == nullptr) {
        return;
    }

    osc->type         = type;
    osc->frequency_hz = frequency_hz;
    osc->phase        = 0.0f;
    osc->active       = true;

    // Advance the global counter and assign to this oscillator's LFSR so each
    // noise voice starts from a unique state.  LCG step gives good seed diversity
    // without adding measurable overhead.
    noise_seed_counter = noise_seed_counter * 1664525u + 1013904223u;
    osc->lfsr = noise_seed_counter | 1u;  // ensure non-zero (xorshift is stuck at 0)
}

void oscillator_note_off(
    OscillatorState* osc
)
{
    if (osc == nullptr) {
        return;
    }

    osc->active = false;
}

static float generate_sine(float phase)
{
    return sinf(phase * 2.0f * (float)M_PI);
}

static float generate_square(float phase)
{
    return phase < 0.5f ? 1.0f : -1.0f;
}

static float generate_saw(float phase)
{
    return (phase * 2.0f) - 1.0f;
}

static float generate_triangle(float phase)
{
    if (phase < 0.25f) {
        return phase * 4.0f;
    }

    if (phase < 0.75f) {
        return 2.0f - (phase * 4.0f);
    }

    return (phase * 4.0f) - 4.0f;
}

// xorshift32 white noise — period 2^32−1 (≈97,000 years at 44.1 kHz).
// Returns a value in −1.0…+1.0.  Caller must ensure osc->lfsr != 0.
static float generate_noise(OscillatorState* osc)
{
    uint32_t x = osc->lfsr;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    osc->lfsr = x;
    return (float)(int32_t)x * (1.0f / 2147483648.0f);
}

float oscillator_generate_sample(
    OscillatorState* osc,
    float sample_rate_hz
)
{
    if (osc == nullptr ||
        !osc->active ||
        sample_rate_hz <= 0.0f) {
        return 0.0f;
    }

    float sample = 0.0f;

    switch (osc->type) {
        case OSC_SINE:
            sample = generate_sine(osc->phase);
            break;

        case OSC_SQUARE:
            sample = generate_square(osc->phase);
            break;

        case OSC_SAW:
            sample = generate_saw(osc->phase);
            break;

        case OSC_TRIANGLE:
            sample = generate_triangle(osc->phase);
            break;

        case OSC_NOISE:
            // Noise does not use the phase accumulator — each sample is
            // independently generated from the LFSR state.
            return generate_noise(osc);

        default:
            sample = 0.0f;
            break;
    }

    osc->phase += osc->frequency_hz / sample_rate_hz;

    while (osc->phase >= 1.0f) {
        osc->phase -= 1.0f;
    }

    return sample;
}