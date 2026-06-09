#pragma once

// ======================================================
// VOICE DEFINITIONS — AUDIO DATA MODEL
//
// A "voice" is a complete sound description: what waveform or audio
// source to play, how the amplitude evolves over time (ADSR), and
// for sample-based voices, which audio files map to which keys.
//
// THE 16-VOICE BANK  (MAX_VOICES = 16, indices 0–15)
//   Voices 0–14  General-purpose oscillator, wavetable, or playback voices.
//   Voice 15     Reserved for the user's own recording (VOICE_RECORD_PLAYBACK).
//                Pre-assigned by voice_definitions_init_defaults() on boot.
//
//   Built-in defaults (loaded at startup without an SD card):
//     0 = Sine oscillator
//     1 = Square oscillator
//     2 = Saw oscillator
//    15 = User recording (PLAYBACK, reads /recordings/user_voice.wav)
//
//   Additional voices can be loaded from a config file on the SD card
//   using voice_config_parser. SD-loaded voices overwrite the defaults
//   at the same slot index.
//
// VOICE TYPES
//   OSCILLATOR  Real-time waveform synthesis via oscillator_engine.
//               The oscillator field picks the waveform; frequency is
//               derived from the MIDI note number at note-on time.
//
//   WAVETABLE   A single looping audio file on the SD card (filename).
//               Playback rate is scaled to pitch the sample to the
//               pressed note relative to the file's root note.
//               Not yet implemented — requires sample_loader.
//
//   PLAYBACK    Up to MAX_SAMPLES_PER_VOICE per-note audio files on
//               the SD card. Each SampleSlot maps one keyboard note
//               index (0–19) to a filename. When a key is pressed the
//               closest assigned slot plays at its natural pitch.
//               Used for the user recording voice (slot 15).
//
// ADSR ENVELOPE
//   Applied per-voice in the synth engine render loop on core 1.
//   Units: attack/decay/release in milliseconds, sustain is 0.0–1.0.
//
//     Attack  ramp 0 → 1.0 over attack_ms after key-down.
//     Decay   ramp 1.0 → sustain_level over decay_ms.
//     Sustain hold at sustain_level while key is held.
//     Release ramp sustain_level → 0 over release_ms after key-up.
//
// SAMPLE SLOTS (PLAYBACK voices only)
//   SampleSlot maps keyboard note_index (0–19) → SD card audio file.
//   Slots are populated by 'sample' lines in the voice config file.
//   Unused slots (assigned=false) are silent when that key is pressed.
// ======================================================

#include <stdint.h>
#include <vector>

#include "audio_constants.h"
#include "../config/firmware_config.h"

static constexpr uint16_t MAX_PATH_LENGTH = 64;
static constexpr uint8_t MAX_VOICE_NAME_LENGTH = 32;

// How the synth generates audio for this voice.
enum VoiceType {
    VOICE_TYPE_OSCILLATOR,  // Real-time waveform synthesis
    VOICE_TYPE_WAVETABLE,   // Single pitched audio file from SD card
    VOICE_TYPE_PLAYBACK     // Per-note audio samples from SD card
};

// Waveform shape for VOICE_TYPE_OSCILLATOR voices.
// Phase accumulator model — all shapes output −1.0 to +1.0.
// OSC_NOISE uses an xorshift32 LFSR instead of a phase accumulator;
// each sample is an independent pseudo-random value (white noise).
enum OscillatorType {
    OSC_NONE,
    OSC_SINE,
    OSC_SQUARE,
    OSC_SAW,
    OSC_TRIANGLE,
    OSC_NOISE
};

// Amplitude envelope applied to every voice on core 1's render loop.
struct AdsrEnvelope {
    uint16_t attack_ms;     // Time from key-down to peak amplitude
    uint16_t decay_ms;      // Time from peak to sustain level
    float    sustain_level; // Held amplitude while key is down (0.0–1.0)
    uint16_t release_ms;    // Time from key-up to silence
};

// One audio file mapped to a specific keyboard note (PLAYBACK voices).
struct SampleSlot {
    bool    assigned;
    uint8_t note_index;                // Keyboard note index (0–19)
    char    filename[MAX_PATH_LENGTH]; // Path on SD card
};

// Per-key sound override for OSCILLATOR voices.
// When a KeyOverride is assigned to a key slot, its parameters replace the
// voice defaults for that key only.  All other keys continue to use the voice.
// This enables drum kits: each key can be a completely different instrument.
//
// fixed_frequency_hz — if > 0, the oscillator plays at this exact frequency
//   regardless of which key is pressed.  Use note/midi/freq in the config file.
//   0.0 = use standard MIDI note-to-frequency mapping (default, melodic use).
struct KeyOverride {
    uint8_t        key_index;           // which key slot this applies to (0–19)
    OscillatorType oscillator;          // primary waveform for this key
    OscillatorType osc2_type;          // second oscillator (OSC_NONE = off)
    int16_t        osc2_detune;        // osc2 cents offset from osc1
    float          pitch_env_depth;    // pitch sweep multiplier (1.0 = off)
    uint16_t       pitch_env_ms;       // pitch sweep duration in ms (0 = off)
    AdsrEnvelope   adsr;               // amplitude envelope for this key
    float          fixed_frequency_hz; // 0.0 = use MIDI note pitch
};

// Complete definition of one voice slot.
struct VoiceDefinition {
    bool        assigned;            // false = slot is empty
    uint8_t     index;               // Slot number (0–15)
    VoiceType   type;
    OscillatorType oscillator;       // Primary oscillator waveform (OSCILLATOR voices)

    // ---- Pitch envelope ----
    // Sweeps the oscillator frequency from (note_freq × pitch_env_depth) down
    // to note_freq over pitch_env_ms milliseconds after note-on.
    // This gives kick drums their characteristic "boom" and toms their "thud".
    //   pitch_env_depth = 1.0 → no sweep (default, zero overhead in render loop)
    //   pitch_env_depth = 4.0 → start 2 octaves above, sweep down to note pitch
    //   pitch_env_ms    = 0   → no sweep regardless of depth
    // Pitch envelope is not applied to OSC_NOISE oscillators.
    float    pitch_env_depth;  // starting frequency multiplier (≥1.0; 1.0 = off)
    uint16_t pitch_env_ms;     // sweep duration in milliseconds (0 = off)

    // ---- Dual-oscillator unison / detune ----
    // When osc2_type != OSC_NONE a second oscillator runs in parallel with
    // the primary.  The two outputs are averaged (×0.5) so peak amplitude
    // is independent of the oscillators' phase relationship.
    //
    // osc2_detune is in cents relative to osc1 (signed, −2400…+2400):
    //   +12   = 12 ¢ sharp  — classic unison detune / chorus fatness
    //   +1200 = one octave above  — upper register layer (organ)
    //   −1200 = one octave below  — sub-bass layer
    //   +1900 = perfect 12th      — bell upper partial
    // PLAYBACK and WAVETABLE voices ignore these fields.
    OscillatorType osc2_type;        // OSC_NONE = single oscillator (default)
    int16_t        osc2_detune;      // cents offset of osc2 from osc1

    char filename[MAX_PATH_LENGTH];  // Root file for WAVETABLE; unused for OSCILLATOR
    char display_name[MAX_VOICE_NAME_LENGTH];

    AdsrEnvelope adsr;

    // Both collections use std::vector so only the entries that are actually
    // configured consume heap memory.  An empty voice costs just 12 bytes per
    // vector (pointer + size + capacity on a 32-bit system) — no static
    // pre-allocation of unused slots.
    std::vector<SampleSlot>  samples;       // PLAYBACK voices: per-note SD card files
    std::vector<KeyOverride> key_overrides; // OSCILLATOR voices: per-key sound overrides
};

// Populate built-in defaults into bank 0.  Banks 1–(MAX_VOICE_BANKS−1)
// start empty; they are populated by voice_config_parser from the SD card.
// Called by voice_manager_init() before any SD card load attempt.
bool voice_definitions_init_defaults();

// Switch the active bank.  All subsequent get/set calls operate on this bank.
// Called by voice_manager_set_bank() and by voice_config_parser when it
// encounters a "bank <n>" directive.
void voice_definitions_set_bank(uint8_t bank);

// Return the currently active bank index.
uint8_t voice_definitions_get_bank();

// Return the number of banks that have at least one assigned voice slot.
// Bank 0 always counts.  Used by voice_manager to determine whether bank
// selection mode is available and how many banks to cycle through.
uint8_t voice_definitions_configured_bank_count();

// Retrieve a voice definition from the active bank by slot index.
// Returns nullptr if the slot is unassigned in the current bank.
const VoiceDefinition* voice_definitions_get(uint8_t voice_index);

// Write a voice definition into the active bank at the slot given by voice.index.
// Called by voice_config_parser while loading from the SD card.
bool voice_definitions_set(const VoiceDefinition& voice);

// Add or replace a SampleSlot in a PLAYBACK voice without copying the whole
// VoiceDefinition.  Matches on slot.note_index; replaces if already present.
bool voice_definitions_add_sample(uint8_t voice_index, const SampleSlot& slot);

// Add or replace a KeyOverride in an OSCILLATOR voice without copying the whole
// VoiceDefinition.  Matches on ko.key_index; replaces if already present.
bool voice_definitions_add_key_override(uint8_t voice_index, const KeyOverride& ko);

// Find the KeyOverride assigned to key_index in the given voice.
// Returns nullptr if no override is assigned for that key.
// Called by synth_engine at note-on time — O(n) scan, n ≤ 20.
const KeyOverride* voice_definitions_find_key_override(
    const VoiceDefinition* voice, uint8_t key_index);

void voice_definitions_print();
void voice_definitions_print_voice(uint8_t voice_index);