#include <stdio.h>
#include <string.h>

#include "audio_constants.h"
#include "voice_definitions.h"

static VoiceDefinition voices[MAX_VOICE_BANKS][MAX_VOICES];
static uint8_t active_bank = 0;

static AdsrEnvelope default_adsr()
{
    return {
        5,      // attack_ms
        100,    // decay_ms
        0.80f,  // sustain_level
        200     // release_ms
    };
}

static void clear_voice(VoiceDefinition& voice)
{
    // Do NOT memset — VoiceDefinition contains std::vector members whose
    // internal state (pointer, size, capacity) must be managed by their
    // own constructors/destructors.  memset would corrupt them silently.
    voice.assigned         = false;
    voice.index            = 0;
    voice.type             = VOICE_TYPE_OSCILLATOR;
    voice.oscillator       = OSC_NONE;
    voice.pitch_env_depth  = 1.0f;
    voice.pitch_env_ms     = 0;
    voice.osc2_type        = OSC_NONE;
    voice.osc2_detune      = 0;
    voice.filename[0]      = '\0';
    voice.display_name[0]  = '\0';
    voice.adsr             = default_adsr();
    voice.samples.clear();
    voice.key_overrides.clear();
}

void voice_definitions_set_bank(uint8_t bank)
{
    if (bank < MAX_VOICE_BANKS) {
        active_bank = bank;
    }
}

uint8_t voice_definitions_get_bank()
{
    return active_bank;
}

uint8_t voice_definitions_configured_bank_count()
{
    uint8_t count = 0;
    for (uint8_t b = 0; b < MAX_VOICE_BANKS; b++) {
        for (uint8_t v = 0; v < MAX_VOICES; v++) {
            if (voices[b][v].assigned) {
                count++;
                break;
            }
        }
    }
    return count;
}

bool voice_definitions_init_defaults()
{
    printf("Initializing default voice definitions...\n");

    // Clear all banks; banks 1+ remain empty until loaded from SD card.
    for (uint8_t b = 0; b < MAX_VOICE_BANKS; b++) {
        for (uint8_t i = 0; i < MAX_VOICES; i++) {
            clear_voice(voices[b][i]);
            voices[b][i].index = i;
        }
    }
    active_bank = 0;

    // ---- Slots 0–3: basic single oscillators ----
    // Convenience alias — all defaults go into bank 0
    VoiceDefinition* v = voices[0];

    v[0].assigned  = true;  v[0].oscillator = OSC_SINE;
    v[0].adsr      = { 5, 100, 0.80f, 200 };
    strncpy(v[0].display_name, "Sine", MAX_VOICE_NAME_LENGTH - 1);

    v[1].assigned  = true;  v[1].oscillator = OSC_SQUARE;
    v[1].adsr      = { 5, 100, 0.80f, 200 };
    strncpy(v[1].display_name, "Square", MAX_VOICE_NAME_LENGTH - 1);

    v[2].assigned  = true;  v[2].oscillator = OSC_SAW;
    v[2].adsr      = { 5, 100, 0.80f, 200 };
    strncpy(v[2].display_name, "Saw", MAX_VOICE_NAME_LENGTH - 1);

    v[3].assigned  = true;  v[3].oscillator = OSC_TRIANGLE;
    v[3].adsr      = { 5, 100, 0.80f, 200 };
    strncpy(v[3].display_name, "Triangle", MAX_VOICE_NAME_LENGTH - 1);

    // ---- Slot 4: Jump Lead — Oberheim OB-Xa / Van Halen "Jump" ----
    v[4].assigned    = true;  v[4].oscillator  = OSC_SAW;
    v[4].osc2_type   = OSC_SAW;  v[4].osc2_detune = 12;
    v[4].adsr        = { 15, 50, 0.88f, 350 };
    strncpy(v[4].display_name, "Jump Lead", MAX_VOICE_NAME_LENGTH - 1);

    // ---- Slot 5: Supersaw — Roland JP-8000 style ----
    v[5].assigned    = true;  v[5].oscillator  = OSC_SAW;
    v[5].osc2_type   = OSC_SAW;  v[5].osc2_detune = 20;
    v[5].adsr        = { 5, 10, 0.95f, 150 };
    strncpy(v[5].display_name, "Supersaw", MAX_VOICE_NAME_LENGTH - 1);

    // ---- Slot 6: Warm Pad ----
    v[6].assigned    = true;  v[6].oscillator  = OSC_TRIANGLE;
    v[6].osc2_type   = OSC_TRIANGLE;  v[6].osc2_detune = 7;
    v[6].adsr        = { 350, 200, 0.85f, 700 };
    strncpy(v[6].display_name, "Warm Pad", MAX_VOICE_NAME_LENGTH - 1);

    // ---- Slot 7: Hollow Lead — pulse-width simulation ----
    v[7].assigned    = true;  v[7].oscillator  = OSC_SQUARE;
    v[7].osc2_type   = OSC_SQUARE;  v[7].osc2_detune = 5;
    v[7].adsr        = { 10, 80, 0.75f, 250 };
    strncpy(v[7].display_name, "Hollow Lead", MAX_VOICE_NAME_LENGTH - 1);

    // ---- Slot 8: Pluck — sustain=0 decays naturally like a plucked string ----
    v[8].assigned   = true;  v[8].oscillator = OSC_TRIANGLE;
    v[8].adsr       = { 2, 300, 0.0f, 80 };
    strncpy(v[8].display_name, "Pluck", MAX_VOICE_NAME_LENGTH - 1);

    // ---- Slot 9: Brass Stab ----
    v[9].assigned   = true;  v[9].oscillator = OSC_SAW;
    v[9].adsr       = { 20, 100, 0.30f, 150 };
    strncpy(v[9].display_name, "Brass Stab", MAX_VOICE_NAME_LENGTH - 1);

    // ---- Slot 10: Sub Bass — saw + octave below ----
    v[10].assigned    = true;  v[10].oscillator  = OSC_SAW;
    v[10].osc2_type   = OSC_SAW;  v[10].osc2_detune = -1200;
    v[10].adsr        = { 5, 100, 0.80f, 200 };
    strncpy(v[10].display_name, "Sub Bass", MAX_VOICE_NAME_LENGTH - 1);

    // ---- Slot 11: Slow Strings — 500ms attack mimics bow-on-string ----
    v[11].assigned    = true;  v[11].oscillator  = OSC_SAW;
    v[11].osc2_type   = OSC_SAW;  v[11].osc2_detune = 8;
    v[11].adsr        = { 500, 200, 0.80f, 800 };
    strncpy(v[11].display_name, "Slow Strings", MAX_VOICE_NAME_LENGTH - 1);

    // ---- Slot 12: Organ — square + octave above, instant on/off ----
    v[12].assigned    = true;  v[12].oscillator  = OSC_SQUARE;
    v[12].osc2_type   = OSC_SQUARE;  v[12].osc2_detune = 1200;
    v[12].adsr        = { 2, 0, 1.0f, 2 };
    strncpy(v[12].display_name, "Organ", MAX_VOICE_NAME_LENGTH - 1);

    // ---- Slot 13: Bell — sine + perfect 12th, decays with no sustain ----
    v[13].assigned    = true;  v[13].oscillator  = OSC_SINE;
    v[13].osc2_type   = OSC_SINE;  v[13].osc2_detune = 1900;
    v[13].adsr        = { 2, 600, 0.0f, 150 };
    strncpy(v[13].display_name, "Bell", MAX_VOICE_NAME_LENGTH - 1);

    // ---- Slot 14: Synth Kit — full 20-key drum kit from pure synthesis ----
    // Each key is independently assigned a drum or cymbal sound via key_overrides.
    // White keys: kick variants, toms, snares, rim shot, clap, side stick.
    // Black keys: cymbals (closed/open hi-hat, crash, ride), cowbell, bells, tambourine.
    // The voice-level oscillator and ADSR act as a silent fallback for any key
    // not explicitly mapped; in practice all 20 keys are overridden below.
    v[14].assigned   = true;
    v[14].oscillator = OSC_NOISE;
    v[14].adsr       = { 2, 100, 0.0f, 40 };
    strncpy(v[14].display_name, "Synth Kit", MAX_VOICE_NAME_LENGTH - 1);

    // Lambda to keep the push_back calls concise and readable.
    // Parameters: key, osc1, osc2, osc2_detune_cents, pitch_depth, pitch_ms,
    //             fixed_freq_hz (0=MIDI pitch), adsr.
    auto drum = [&](uint8_t key,
                    OscillatorType osc1, OscillatorType osc2, int16_t det,
                    float depth, uint16_t pitch_ms, float freq,
                    AdsrEnvelope adsr)
    {
        KeyOverride ko{};
        ko.key_index          = key;
        ko.oscillator         = osc1;
        ko.osc2_type          = osc2;
        ko.osc2_detune        = det;
        ko.pitch_env_depth    = depth;
        ko.pitch_env_ms       = pitch_ms;
        ko.fixed_frequency_hz = freq;
        ko.adsr               = adsr;
        v[14].key_overrides.push_back(ko);
    };

    // Keyboard layout (F3–C5, 20 keys, low→high):
    //  Key  Note  Color  Drum sound
    //   0   F3    white  Sub Kick        11   E4   white  Snare
    //   1   F#3   BLACK  Cowbell         12   F4   white  Snare Lo
    //   2   G3    white  Kick            13   F#4  BLACK  Bell Hi
    //   3   G#3   BLACK  Crash           14   G4   white  Rim Shot
    //   4   A3    white  Kick Hi         15   G#4  BLACK  Bell Lo
    //   5   A#3   BLACK  Open HiHat      16   A4   white  Clap
    //   6   B3    white  Floor Tom       17   A#4  BLACK  Tambourine
    //   7   C4    white  Mid Tom         18   B4   white  Hand Clap
    //   8   C#4   BLACK  Closed HiHat    19   C5   white  Side Stick
    //   9   D4    white  Hi Tom
    //  10   D#4   BLACK  Ride

    //            key  osc1        osc2          det   dpth  ms   Hz      A   D     S    R
    // White — kicks (sine + pitch sweep, fixed low frequency)
    drum(  0, OSC_SINE,     OSC_NONE,       0, 4.0f,  90,  65.4f, {  2, 350, 0.0f,  50 }); // Sub Kick  C2
    drum(  2, OSC_SINE,     OSC_NONE,       0, 3.5f,  75,  82.4f, {  2, 280, 0.0f,  50 }); // Kick      E2
    drum(  4, OSC_SINE,     OSC_NONE,       0, 3.0f,  60,  98.0f, {  2, 200, 0.0f,  40 }); // Kick Hi   G2
    // White — toms (triangle + pitch sweep)
    drum(  6, OSC_TRIANGLE, OSC_NONE,       0, 2.5f,  60, 110.0f, {  2, 280, 0.0f,  60 }); // Floor Tom A2
    drum(  7, OSC_TRIANGLE, OSC_NONE,       0, 2.0f,  50, 146.8f, {  2, 230, 0.0f,  50 }); // Mid Tom   D3
    drum(  9, OSC_TRIANGLE, OSC_NONE,       0, 2.0f,  40, 196.0f, {  2, 180, 0.0f,  40 }); // Hi Tom    G3
    // White — snares (noise + triangle body at fixed pitch)
    drum( 11, OSC_NOISE,    OSC_TRIANGLE,   0, 1.0f,   0, 146.8f, {  2, 120, 0.0f,  40 }); // Snare     D3 body
    drum( 12, OSC_NOISE,    OSC_TRIANGLE,   0, 1.0f,   0, 110.0f, {  2, 180, 0.0f,  60 }); // Snare Lo  A2 body
    // White — hits
    drum( 14, OSC_SQUARE,   OSC_NONE,       0, 1.0f,   0, 440.0f, {  1,  50, 0.0f,  20 }); // Rim Shot  A4
    drum( 16, OSC_NOISE,    OSC_NONE,       0, 1.0f,   0,   0.0f, {  2,  80, 0.0f,  30 }); // Clap
    drum( 18, OSC_NOISE,    OSC_NONE,       0, 1.0f,   0,   0.0f, {  3, 100, 0.0f,  50 }); // Hand Clap
    drum( 19, OSC_SQUARE,   OSC_NONE,       0, 1.0f,   0, 493.9f, {  1,  30, 0.0f,  10 }); // Side Stick B4
    // Black — cowbell (TR-808: 562Hz + 845Hz ≈ 706 ¢ apart)
    drum(  1, OSC_SQUARE,   OSC_SQUARE,   706, 1.0f,   0, 562.0f, {  2, 400, 0.0f, 150 }); // Cowbell
    // Black — cymbals (noise, varying decay)
    drum(  3, OSC_NOISE,    OSC_NONE,       0, 1.0f,   0,   0.0f, {  1, 800, 0.0f, 400 }); // Crash
    drum(  5, OSC_NOISE,    OSC_NONE,       0, 1.0f,   0,   0.0f, {  1, 300, 0.0f, 150 }); // Open HiHat
    drum(  8, OSC_NOISE,    OSC_NONE,       0, 1.0f,   0,   0.0f, {  1,  40, 0.0f,  10 }); // Closed HiHat
    drum( 10, OSC_NOISE,    OSC_NONE,       0, 1.0f,   0,   0.0f, {  1, 500, 0.0f, 200 }); // Ride
    // Black — bells (sine + perfect 12th at fixed frequencies)
    drum( 13, OSC_SINE,     OSC_SINE,    1900, 1.0f,   0, 880.0f, {  2, 600, 0.0f, 200 }); // Bell Hi   A5
    drum( 15, OSC_SINE,     OSC_SINE,    1900, 1.0f,   0, 440.0f, {  2, 700, 0.0f, 250 }); // Bell Lo   A4
    // Black — tambourine (short noise burst)
    drum( 17, OSC_NOISE,    OSC_NONE,       0, 1.0f,   0,   0.0f, {  1,  60, 0.0f,  20 }); // Tambourine

    // ---- Slot 15: User recording (reserved) ----
    v[15].assigned = true;
    v[15].type     = VOICE_TYPE_PLAYBACK;
    strncpy(v[15].filename, "/recordings/user_voice.wav", MAX_PATH_LENGTH - 1);
    strncpy(v[15].display_name, "User Recording", MAX_VOICE_NAME_LENGTH - 1);

    printf("Default voice definitions initialized.\n");

    return true;
}

const VoiceDefinition* voice_definitions_get(uint8_t voice_index)
{
    if (voice_index >= MAX_VOICES) return nullptr;
    if (!voices[active_bank][voice_index].assigned) return nullptr;
    return &voices[active_bank][voice_index];
}

bool voice_definitions_set(const VoiceDefinition& voice)
{
    if (voice.index >= MAX_VOICES) return false;
    voices[active_bank][voice.index] = voice;
    voices[active_bank][voice.index].assigned = true;
    return true;
}

bool voice_definitions_add_sample(uint8_t voice_index, const SampleSlot& slot)
{
    if (voice_index >= MAX_VOICES) return false;
    VoiceDefinition& v = voices[active_bank][voice_index];
    if (!v.assigned) return false;
    for (auto& s : v.samples) {
        if (s.note_index == slot.note_index) { s = slot; return true; }
    }
    v.samples.push_back(slot);
    return true;
}

bool voice_definitions_add_key_override(uint8_t voice_index, const KeyOverride& ko)
{
    if (voice_index >= MAX_VOICES) return false;
    VoiceDefinition& v = voices[active_bank][voice_index];
    if (!v.assigned) return false;
    for (auto& existing : v.key_overrides) {
        if (existing.key_index == ko.key_index) { existing = ko; return true; }
    }
    v.key_overrides.push_back(ko);
    return true;
}

const KeyOverride* voice_definitions_find_key_override(
    const VoiceDefinition* voice, uint8_t key_index)
{
    if (!voice) return nullptr;
    for (const auto& ko : voice->key_overrides) {
        if (ko.key_index == key_index) return &ko;
    }
    return nullptr;
}

static const char* voice_type_name(VoiceType type)
{
    switch (type) {
        case VOICE_TYPE_OSCILLATOR: return "OSC";
        case VOICE_TYPE_WAVETABLE:  return "WAVETABLE";
        case VOICE_TYPE_PLAYBACK:   return "PLAYBACK";
        default:                    return "UNKNOWN";
    }
}

static const char* oscillator_type_name(OscillatorType type)
{
    switch (type) {
        case OSC_SINE:     return "SINE";
        case OSC_SQUARE:   return "SQUARE";
        case OSC_SAW:      return "SAW";
        case OSC_TRIANGLE: return "TRIANGLE";
        case OSC_NOISE:    return "NOISE";
        default:           return "NONE";
    }
}

void voice_definitions_print()
{
    printf("Voice definitions (bank %u):\n", active_bank);

    for (uint8_t i = 0; i < MAX_VOICES; i++) {
        const VoiceDefinition& v = voices[active_bank][i];

        if (!v.assigned) continue;

        printf("  %2u  %-9s  %-16s",
               v.index,
               voice_type_name(v.type),
               v.display_name);

        if (v.type == VOICE_TYPE_OSCILLATOR) {
            printf(" osc=%s", oscillator_type_name(v.oscillator));
        }

        if (v.filename[0] != '\0') {
            printf(" file=%s", v.filename);
        }

        printf("\n");
    }
}

void voice_definitions_print_voice(uint8_t voice_index)
{
    const VoiceDefinition* voice = voice_definitions_get(voice_index);

    if (voice == nullptr) {
        printf("Voice %u is not assigned.\n", voice_index);
        return;
    }

    printf("Voice %u\n", voice->index);
    printf("  Name    : %s\n", voice->display_name);
    printf("  Type    : %s\n", voice_type_name(voice->type));
    printf("  Osc     : %s\n", oscillator_type_name(voice->oscillator));
    printf("  File    : %s\n", voice->filename[0] ? voice->filename : "(none)");

    printf("  ADSR    : A=%ums D=%ums S=%.2f R=%ums\n",
           voice->adsr.attack_ms,
           voice->adsr.decay_ms,
           voice->adsr.sustain_level,
           voice->adsr.release_ms);

    if (voice->type == VOICE_TYPE_PLAYBACK && !voice->samples.empty()) {
        printf("  Samples (%u):\n", (unsigned)voice->samples.size());
        for (const auto& s : voice->samples) {
            printf("    note %u -> %s\n", s.note_index, s.filename);
        }
    }

    if (!voice->key_overrides.empty()) {
        printf("  Key overrides (%u):\n", (unsigned)voice->key_overrides.size());
        for (const auto& ko : voice->key_overrides) {
            printf("    key %u  osc=%s  freq=%.1f\n",
                   ko.key_index,
                   oscillator_type_name(ko.oscillator),
                   ko.fixed_frequency_hz);
        }
    }
}