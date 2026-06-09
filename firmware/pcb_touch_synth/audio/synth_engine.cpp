#include <stdio.h>
#include <string.h>
#include <math.h>

#include "audio_constants.h"
#include "music_theory.h"
#include "oscillator_engine.h"
#include "sample_loader.h"
#include "synth_engine.h"
#include "voice_definitions.h"

// ======================================================
// ACTIVE NOTE STATE
//
// One slot per simultaneous note. Slots are recycled on note-off
// only after the ADSR release tail reaches silence, so the release
// envelope plays out naturally even after the key is lifted.
// ======================================================

struct ActiveNote {
    AdsrPhase adsr_phase;
    float     adsr_level;        // current envelope amplitude (0.0–1.0)
    float     adsr_start_level;  // level at the start of the RELEASE phase
    uint32_t  adsr_phase_samples;// samples elapsed in the current ADSR phase

    uint8_t note_index;
    uint8_t midi_note;
    uint8_t voice_index;
    VoiceType voice_type;

    // OSCILLATOR voices
    OscillatorState oscillator;
    OscillatorState oscillator2;   // second oscillator; only used when dual_osc == true
    bool            dual_osc;      // true when voice->osc2_type != OSC_NONE
    float           osc2_detune_ratio; // 2^(osc2_detune/1200), computed once at note-on

    // Pitch envelope — sweeps oscillator frequency from (note_freq × depth)
    // down to note_freq over pitch_env_ms.  Inactive when total_samples == 0.
    // Not applied to OSC_NOISE oscillators (noise has no fundamental pitch).
    float    pitch_start_hz;          // note_freq × pitch_env_depth
    float    pitch_end_hz;            // note_freq (target)
    uint32_t pitch_env_total_samples; // 0 = no envelope
    uint32_t pitch_env_samples;       // samples elapsed since note-on

    // ADSR resolved at note-on: either from the voice default or a key override.
    // Stored here so fill_buffer never needs to re-examine the key override list.
    AdsrEnvelope effective_adsr;

    // PLAYBACK voices
    const int32_t* sample_data;    // pointer into SRAM pool (from sample_loader)
    uint32_t       sample_length;  // total sample frames
    float          sample_position;// fractional position for pitch shifting
    float          sample_pitch_ratio; // playback_rate / AUDIO_SAMPLE_RATE_HZ
};

static ActiveNote active_notes[MAX_ACTIVE_NOTES];


// ======================================================
// ADSR HELPERS
// ======================================================

// Convert milliseconds to sample frames at AUDIO_SAMPLE_RATE_HZ.
static inline float ms_to_samples(uint16_t ms)
{
    return (float)ms * (AUDIO_SAMPLE_RATE_HZ / 1000.0f);
}

// Advance one note's ADSR envelope by one sample frame.
// Updates adsr_level and adsr_phase. If the release tail completes,
// adsr_phase is set to ADSR_IDLE and the note becomes free.
// Returns the current envelope level (0.0–1.0).
static float adsr_advance(ActiveNote& note, const AdsrEnvelope& env)
{
    note.adsr_phase_samples++;

    switch (note.adsr_phase) {

        case ADSR_ATTACK: {
            float duration = ms_to_samples(env.attack_ms);
            if (duration <= 0.0f) {
                note.adsr_level = 1.0f;
                note.adsr_phase = ADSR_DECAY;
                note.adsr_phase_samples = 0;
            } else {
                note.adsr_level = (float)note.adsr_phase_samples / duration;
                if (note.adsr_level >= 1.0f) {
                    note.adsr_level = 1.0f;
                    note.adsr_phase = ADSR_DECAY;
                    note.adsr_phase_samples = 0;
                }
            }
            break;
        }

        case ADSR_DECAY: {
            float duration = ms_to_samples(env.decay_ms);
            if (duration <= 0.0f) {
                note.adsr_level = env.sustain_level;
                note.adsr_phase = ADSR_SUSTAIN;
                note.adsr_phase_samples = 0;
            } else {
                float progress = (float)note.adsr_phase_samples / duration;
                note.adsr_level = 1.0f - progress * (1.0f - env.sustain_level);
                if (progress >= 1.0f) {
                    note.adsr_level = env.sustain_level;
                    note.adsr_phase = ADSR_SUSTAIN;
                    note.adsr_phase_samples = 0;
                }
            }
            break;
        }

        case ADSR_SUSTAIN:
            // Level stays constant until note_off transitions to RELEASE.
            note.adsr_level = env.sustain_level;
            break;

        case ADSR_RELEASE: {
            float duration = ms_to_samples(env.release_ms);
            if (duration <= 0.0f) {
                note.adsr_level = 0.0f;
                note.adsr_phase = ADSR_IDLE;
            } else {
                float progress = (float)note.adsr_phase_samples / duration;
                note.adsr_level = note.adsr_start_level * (1.0f - progress);
                if (progress >= 1.0f || note.adsr_level <= 0.0f) {
                    note.adsr_level = 0.0f;
                    note.adsr_phase = ADSR_IDLE;
                }
            }
            break;
        }

        case ADSR_IDLE:
            note.adsr_level = 0.0f;
            break;
    }

    return note.adsr_level;
}


// ======================================================
// PUBLIC API
// ======================================================

bool synth_engine_init()
{
    printf("Initializing synth engine...\n");

    voice_definitions_init_defaults();

    for (uint8_t i = 0; i < MAX_ACTIVE_NOTES; i++) {
        active_notes[i].adsr_phase = ADSR_IDLE;
    }

    printf("Synth engine initialized.\n");
    return true;
}

void synth_engine_note_on(uint8_t note_index, uint8_t midi_note, uint8_t voice_index)
{
    const VoiceDefinition* voice = voice_definitions_get(voice_index);

    if (voice == nullptr) {
        printf("SYNTH: note_on — voice %u not assigned\n", voice_index);
        return;
    }

    // Find a free slot (ADSR_IDLE).
    for (uint8_t i = 0; i < MAX_ACTIVE_NOTES; i++) {
        if (active_notes[i].adsr_phase != ADSR_IDLE) {
            continue;
        }

        ActiveNote& note = active_notes[i];

        note.note_index  = note_index;
        note.midi_note   = midi_note;
        note.voice_index = voice_index;
        note.voice_type  = voice->type;

        // ADSR — start attack phase.
        note.adsr_phase         = ADSR_ATTACK;
        note.adsr_level         = 0.0f;
        note.adsr_start_level   = 0.0f;
        note.adsr_phase_samples = 0;
        note.dual_osc           = false;

        // Voice-type-specific setup.
        switch (voice->type) {

            case VOICE_TYPE_OSCILLATOR: {
                // Resolve per-key override (O(n), n ≤ 20, called only at note-on).
                const KeyOverride* ko =
                    voice_definitions_find_key_override(voice, note_index);

                OscillatorType osc1_type   = ko ? ko->oscillator      : voice->oscillator;
                OscillatorType osc2_type   = ko ? ko->osc2_type       : voice->osc2_type;
                int16_t        osc2_detune = ko ? ko->osc2_detune     : voice->osc2_detune;
                float          pitch_depth = ko ? ko->pitch_env_depth : voice->pitch_env_depth;
                uint16_t       pitch_ms    = ko ? ko->pitch_env_ms    : voice->pitch_env_ms;
                float          fixed_freq  = ko ? ko->fixed_frequency_hz : 0.0f;
                note.effective_adsr        = ko ? ko->adsr            : voice->adsr;

                if (osc1_type == OSC_NONE) {
                    printf("SYNTH: note_on — voice %u has no oscillator type\n", voice_index);
                    note.adsr_phase = ADSR_IDLE;
                    return;
                }

                // Fixed frequency (drums) overrides MIDI pitch mapping.
                float base_freq = (fixed_freq > 0.0f)
                                  ? fixed_freq
                                  : midi_note_to_frequency(midi_note);

                // Pitch envelope: compute once, hot path only does lerp.
                note.pitch_end_hz = base_freq;
                if (pitch_ms > 0 && pitch_depth != 1.0f && osc1_type != OSC_NOISE) {
                    note.pitch_start_hz          = base_freq * pitch_depth;
                    note.pitch_env_total_samples = (uint32_t)(
                        pitch_ms * (AUDIO_SAMPLE_RATE_HZ / 1000.0f));
                } else {
                    note.pitch_start_hz          = base_freq;
                    note.pitch_env_total_samples = 0;
                }
                note.pitch_env_samples = 0;

                float start_freq = (note.pitch_env_total_samples > 0)
                                   ? note.pitch_start_hz : base_freq;
                oscillator_note_on(&note.oscillator, osc1_type, start_freq);

                // Second oscillator: store detune ratio once to avoid powf per sample.
                note.dual_osc = (osc2_type != OSC_NONE);
                if (note.dual_osc) {
                    note.osc2_detune_ratio = powf(2.0f, osc2_detune / 1200.0f);
                    oscillator_note_on(&note.oscillator2, osc2_type,
                                       start_freq * note.osc2_detune_ratio);
                } else {
                    note.osc2_detune_ratio = 1.0f;
                }
                break;
            }

            case VOICE_TYPE_PLAYBACK: {
                uint32_t num_frames  = 0;
                uint32_t sample_rate = 0;
                const int32_t* data  = sample_loader_get_sample(
                    voice_index, note_index, &num_frames, &sample_rate
                );

                if (data == nullptr || num_frames == 0) {
                    printf("SYNTH: note_on — no sample for voice %u note %u\n",
                           voice_index, note_index);
                    note.adsr_phase = ADSR_IDLE;
                    return;
                }

                note.sample_data     = data;
                note.sample_length   = num_frames;
                note.sample_position = 0.0f;
                // Pitch ratio: if sample was recorded at AUDIO_SAMPLE_RATE_HZ,
                // ratio = 1.0. Adjust if sample rates differ.
                note.sample_pitch_ratio = (sample_rate > 0)
                    ? (float)sample_rate / AUDIO_SAMPLE_RATE_HZ
                    : 1.0f;
                break;
            }

            case VOICE_TYPE_WAVETABLE:
                // TODO: load wavetable lookup table from sample_loader.
                printf("SYNTH: note_on — WAVETABLE not yet implemented\n");
                note.adsr_phase = ADSR_IDLE;
                return;
        }

        printf("SYNTH: note on  slot=%u note=%u midi=%u voice=%u type=%d\n",
               i, note_index, midi_note, voice_index, (int)voice->type);
        return;
    }

    printf("SYNTH: note_on — all %u slots busy, note dropped\n", MAX_ACTIVE_NOTES);
}

void synth_engine_note_off(uint8_t note_index, uint8_t midi_note)
{
    for (uint8_t i = 0; i < MAX_ACTIVE_NOTES; i++) {
        ActiveNote& note = active_notes[i];

        if (note.adsr_phase == ADSR_IDLE) continue;
        if (note.note_index != note_index) continue;
        if (note.midi_note  != midi_note)  continue;

        // Transition to RELEASE at the current level (may be in decay/sustain).
        note.adsr_start_level   = note.adsr_level;
        note.adsr_phase         = ADSR_RELEASE;
        note.adsr_phase_samples = 0;

        printf("SYNTH: note off slot=%u note=%u midi=%u level=%.2f\n",
               i, note_index, midi_note, note.adsr_level);
        return;
    }

    printf("SYNTH: note off not found note=%u midi=%u\n", note_index, midi_note);
}

bool synth_engine_notes_active()
{
    for (uint8_t i = 0; i < MAX_ACTIVE_NOTES; i++) {
        if (active_notes[i].adsr_phase != ADSR_IDLE) {
            return true;
        }
    }
    return false;
}

void synth_engine_fill_buffer(int32_t* buffer, uint32_t num_frames)
{
    // Zero the output buffer before accumulating voices.
    for (uint32_t f = 0; f < num_frames; f++) {
        buffer[f] = 0;
    }

    for (uint8_t i = 0; i < MAX_ACTIVE_NOTES; i++) {
        ActiveNote& note = active_notes[i];

        if (note.adsr_phase == ADSR_IDLE) {
            continue;
        }

        const VoiceDefinition* voice = voice_definitions_get(note.voice_index);
        if (voice == nullptr) {
            note.adsr_phase = ADSR_IDLE;
            continue;
        }

        for (uint32_t f = 0; f < num_frames; f++) {
            // Advance ADSR using the envelope resolved at note-on (may be a
            // key override's ADSR rather than the voice default).
            float env = adsr_advance(note, note.effective_adsr);

            if (note.adsr_phase == ADSR_IDLE) {
                // Release tail finished — zero any remaining frames for this slot.
                break;
            }

            float raw = 0.0f;

            switch (note.voice_type) {

                case VOICE_TYPE_OSCILLATOR:
                    // Apply pitch envelope: linearly interpolate frequency from
                    // pitch_start_hz → pitch_end_hz over pitch_env_total_samples.
                    // Guard skips immediately for voices with no pitch envelope (total=0).
                    if (note.pitch_env_total_samples > 0 &&
                        note.pitch_env_samples < note.pitch_env_total_samples) {
                        float t = (float)note.pitch_env_samples
                                / (float)note.pitch_env_total_samples;
                        float swept_hz = note.pitch_start_hz
                                       + (note.pitch_end_hz - note.pitch_start_hz) * t;
                        note.oscillator.frequency_hz = swept_hz;
                        if (note.dual_osc && note.oscillator2.type != OSC_NOISE) {
                            note.oscillator2.frequency_hz = swept_hz * note.osc2_detune_ratio;
                        }
                        note.pitch_env_samples++;
                    }

                    raw = oscillator_generate_sample(&note.oscillator, AUDIO_SAMPLE_RATE_HZ);
                    if (note.dual_osc) {
                        float raw2 = oscillator_generate_sample(&note.oscillator2, AUDIO_SAMPLE_RATE_HZ);
                        raw = (raw + raw2) * 0.5f;
                    }
                    break;

                case VOICE_TYPE_PLAYBACK: {
                    uint32_t pos = (uint32_t)note.sample_position;
                    if (pos >= note.sample_length) {
                        // Sample ended — move to release phase.
                        note.adsr_start_level   = note.adsr_level;
                        note.adsr_phase         = ADSR_RELEASE;
                        note.adsr_phase_samples = 0;
                        raw = 0.0f;
                    } else {
                        // Normalise 24-bit int sample to -1.0..+1.0 float.
                        raw = (float)note.sample_data[pos] / (float)0x7FFFFF;
                        note.sample_position += note.sample_pitch_ratio;
                    }
                    break;
                }

                default:
                    raw = 0.0f;
                    break;
            }

            // Apply envelope and accumulate into output buffer.
            // Scale to 24-bit range and clamp to prevent overflow from multiple voices.
            float scaled = raw * env * (float)0x7FFFFF;
            int64_t accumulated = (int64_t)buffer[f] + (int64_t)scaled;

            if (accumulated >  0x7FFFFF) accumulated =  0x7FFFFF;
            if (accumulated < -0x800000) accumulated = -0x800000;

            buffer[f] = (int32_t)accumulated;
        }
    }
}

void synth_engine_task()
{
    // Per-tick housekeeping. fill_buffer() handles the per-sample work.
    // This can be used for slower-rate tasks like pitch LFO or vibrato.
}

void synth_engine_print_status()
{
    printf("Synth active notes:\n");

    bool any = false;
    for (uint8_t i = 0; i < MAX_ACTIVE_NOTES; i++) {
        const ActiveNote& note = active_notes[i];
        if (note.adsr_phase == ADSR_IDLE) continue;

        static const char* phase_names[] = {
            "IDLE", "ATTACK", "DECAY", "SUSTAIN", "RELEASE"
        };

        printf("  slot=%u note=%u midi=%u voice=%u adsr=%s level=%.2f\n",
               i,
               note.note_index,
               note.midi_note,
               note.voice_index,
               phase_names[(int)note.adsr_phase],
               note.adsr_level);
        any = true;
    }

    if (!any) {
        printf("  (none)\n");
    }
}
