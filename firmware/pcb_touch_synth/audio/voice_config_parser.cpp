#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "voice_config_parser.h"
#include "voice_definitions.h"
#include "music_theory.h"
#include "../config/firmware_config.h"

static uint16_t parsed_lines = 0;
static uint16_t loaded_voices = 0;
static uint16_t loaded_samples = 0;
static uint16_t warning_count = 0;

static void warn(uint32_t line_number, const char* message)
{
    warning_count++;
    printf("VOICE CONFIG warning line %lu: %s\n",
           (unsigned long)line_number,
           message);
}

static void trim_leading(char** text)
{
    while (**text == ' ' || **text == '\t') {
        (*text)++;
    }
}

static void strip_trailing(char* text)
{
    size_t len = strlen(text);

    while (len > 0) {
        char c = text[len - 1];

        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            text[len - 1] = '\0';
            len--;
        } else {
            break;
        }
    }
}

static bool parse_u8(const char* text, uint8_t* value)
{
    if (text == nullptr || value == nullptr || text[0] == '\0') {
        return false;
    }

    char* end = nullptr;
    long parsed = strtol(text, &end, 10);

    if (*end != '\0') {
        return false;
    }

    if (parsed < 0 || parsed > 255) {
        return false;
    }

    *value = (uint8_t)parsed;
    return true;
}

static OscillatorType parse_oscillator_type(const char* text)
{
    if (strcmp(text, "sine") == 0) {
        return OSC_SINE;
    }

    if (strcmp(text, "square") == 0) {
        return OSC_SQUARE;
    }

    if (strcmp(text, "saw") == 0) {
        return OSC_SAW;
    }

    if (strcmp(text, "triangle") == 0) {
        return OSC_TRIANGLE;
    }

    return OSC_NONE;
}

static bool parse_adsr_from_tokens(
    char** tokens,
    uint8_t token_count,
    uint8_t start_index,
    AdsrEnvelope* adsr
)
{
    if (adsr == nullptr) {
        return false;
    }

    if (start_index + 4 >= token_count) {
        return false;
    }

    adsr->attack_ms = (uint16_t)atoi(tokens[start_index + 1]);
    adsr->decay_ms = (uint16_t)atoi(tokens[start_index + 2]);
    adsr->sustain_level = (float)atof(tokens[start_index + 3]);
    adsr->release_ms = (uint16_t)atoi(tokens[start_index + 4]);

    if (adsr->sustain_level < 0.0f) {
        adsr->sustain_level = 0.0f;
    }

    if (adsr->sustain_level > 1.0f) {
        adsr->sustain_level = 1.0f;
    }

    return true;
}

static void parse_name_from_tokens(
    char** tokens,
    uint8_t token_count,
    VoiceDefinition* voice
)
{
    if (voice == nullptr) return;

    for (uint8_t i = 0; i < token_count; i++) {
        if (strcmp(tokens[i], "name") != 0) continue;
        if (i + 1 >= token_count) return;

        // 'name' must be the last keyword on the line.  Every remaining token
        // is joined with spaces to form the display name.  This allows
        // multi-word names ("Jump Lead", "Slow Strings") without quoting.
        // Placing 'name' before other keywords (adsr, osc2) would absorb
        // them into the name string — always put 'name' last.
        voice->display_name[0] = '\0';
        for (uint8_t j = i + 1; j < token_count; j++) {
            if (j > i + 1) {
                strncat(voice->display_name, " ",
                        MAX_VOICE_NAME_LENGTH - 1 - strlen(voice->display_name));
            }
            strncat(voice->display_name, tokens[j],
                    MAX_VOICE_NAME_LENGTH - 1 - strlen(voice->display_name));
        }
        return;
    }
}

static bool tokenize(
    char* line,
    char** tokens,
    uint8_t max_tokens,
    uint8_t* token_count
)
{
    *token_count = 0;

    char* token = strtok(line, " \t");

    while (token != nullptr) {
        if (*token_count >= max_tokens) {
            return false;
        }

        tokens[*token_count] = token;
        (*token_count)++;

        token = strtok(nullptr, " \t");
    }

    return true;
}

// Convert a note name string (e.g. "C4", "A#3", "Bb2", "G-1") to Hz.
// Letter A–G (case-insensitive), optional # or b accidental, then octave number.
// Uses standard MIDI numbering: C4 = 60 = 261.63 Hz, A4 = 69 = 440 Hz.
static bool parse_note_name(const char* text, float* freq_out)
{
    if (!text || !freq_out || text[0] == '\0') return false;

    int semitone = -1;
    switch (text[0] | 0x20) {
        case 'c': semitone = 0;  break;
        case 'd': semitone = 2;  break;
        case 'e': semitone = 4;  break;
        case 'f': semitone = 5;  break;
        case 'g': semitone = 7;  break;
        case 'a': semitone = 9;  break;
        case 'b': semitone = 11; break;
        default:  return false;
    }

    uint8_t pos = 1;
    if (text[pos] == '#') { semitone++;   pos++; }
    else if (text[pos] == 'b') { semitone--; pos++; }
    if (semitone < 0)  semitone += 12;
    if (semitone > 11) semitone -= 12;

    if (text[pos] == '\0') return false;
    char* end = nullptr;
    long octave = strtol(text + pos, &end, 10);
    if (end == text + pos || *end != '\0') return false;
    if (octave < -1 || octave > 9) return false;

    int midi = (int)(octave + 1) * 12 + semitone;
    if (midi < 0 || midi > 127) return false;

    *freq_out = midi_note_to_frequency((uint8_t)midi);
    return true;
}

// Scan tokens for a frequency specifier: "note NAME", "midi NUM", or "freq HZ".
// Sets *freq_out to Hz, or 0.0 if none found (0.0 = use MIDI note pitch).
static bool parse_frequency_from_tokens(
    char** tokens, uint8_t token_count, uint32_t line_number, float* freq_out)
{
    *freq_out = 0.0f;
    for (uint8_t i = 0; i < token_count; i++) {
        if (strcmp(tokens[i], "note") == 0) {
            if (i + 1 >= token_count) {
                warn(line_number, "note requires a name e.g. C4 A#3 Bb2");
                return false;
            }
            if (!parse_note_name(tokens[i + 1], freq_out)) {
                warn(line_number, "invalid note name — use A-G with optional #/b and octave (-1..9)");
                return false;
            }
            return true;
        }
        if (strcmp(tokens[i], "midi") == 0) {
            if (i + 1 >= token_count) {
                warn(line_number, "midi requires a note number 0-127");
                return false;
            }
            int n = atoi(tokens[i + 1]);
            if (n < 0 || n > 127) { warn(line_number, "midi note must be 0-127"); return false; }
            *freq_out = midi_note_to_frequency((uint8_t)n);
            return true;
        }
        if (strcmp(tokens[i], "freq") == 0) {
            if (i + 1 >= token_count) {
                warn(line_number, "freq requires a Hz value");
                return false;
            }
            float hz = (float)atof(tokens[i + 1]);
            if (hz <= 0.0f) { warn(line_number, "freq must be > 0 Hz"); return false; }
            *freq_out = hz;
            return true;
        }
    }
    return true;  // not specified — caller uses 0.0 = MIDI pitch
}

// Parse: key <voice> <key_index> oscillator <waveform> [options...] [name N]
// Options (all keyword-scanned, order-independent except name must be last):
//   osc2 <waveform> <detune_cents>
//   pitch <depth> <ms>
//   note <name> | midi <num> | freq <hz>
//   adsr <attack_ms> <decay_ms> <sustain> <release_ms>
//   name <words to end of line>
static bool parse_key_line(char** tokens, uint8_t token_count, uint32_t line_number)
{
    if (token_count < 5) {
        warn(line_number, "key requires: voice_index key_index oscillator <waveform>");
        return false;
    }

    uint8_t voice_index = 0, key_index = 0;
    if (!parse_u8(tokens[1], &voice_index) || voice_index >= MAX_VOICES) {
        warn(line_number, "invalid voice index (0-15)");
        return false;
    }
    if (!parse_u8(tokens[2], &key_index) || key_index >= MAX_SAMPLES_PER_VOICE) {
        warn(line_number, "invalid key index (0-19)");
        return false;
    }
    if (strcmp(tokens[3], "oscillator") != 0) {
        warn(line_number, "key: expected 'oscillator' as 4th token");
        return false;
    }
    OscillatorType osc1 = parse_oscillator_type(tokens[4]);
    if (osc1 == OSC_NONE) {
        warn(line_number, "unknown waveform for key override");
        return false;
    }

    KeyOverride ko;
    ko.key_index          = key_index;
    ko.oscillator         = osc1;
    ko.osc2_type          = OSC_NONE;
    ko.osc2_detune        = 0;
    ko.pitch_env_depth    = 1.0f;
    ko.pitch_env_ms       = 0;
    ko.fixed_frequency_hz = 0.0f;
    ko.adsr               = { DEFAULT_ATTACK_MS, DEFAULT_DECAY_MS,
                               DEFAULT_SUSTAIN, DEFAULT_RELEASE_MS };

    for (uint8_t i = 5; i < token_count; i++) {
        if (strcmp(tokens[i], "osc2") == 0) {
            if (i + 2 >= token_count) {
                warn(line_number, "osc2 requires <waveform> <detune_cents>");
                return false;
            }
            ko.osc2_type = parse_oscillator_type(tokens[i + 1]);
            if (ko.osc2_type == OSC_NONE) {
                warn(line_number, "unknown osc2 waveform");
                return false;
            }
            char* end;
            long d = strtol(tokens[i + 2], &end, 10);
            if (*end != '\0' || d < -2400 || d > 2400) {
                warn(line_number, "osc2 detune must be -2400..+2400 cents");
                return false;
            }
            ko.osc2_detune = (int16_t)d;
        }
        else if (strcmp(tokens[i], "pitch") == 0) {
            if (i + 2 >= token_count) {
                warn(line_number, "pitch requires <depth> <ms>");
                return false;
            }
            ko.pitch_env_depth = (float)atof(tokens[i + 1]);
            ko.pitch_env_ms    = (uint16_t)atoi(tokens[i + 2]);
            if (ko.pitch_env_depth <= 0.0f) {
                warn(line_number, "pitch depth must be > 0");
                return false;
            }
        }
        else if (strcmp(tokens[i], "adsr") == 0) {
            if (!parse_adsr_from_tokens(tokens, token_count, i, &ko.adsr)) {
                warn(line_number, "invalid ADSR values");
                return false;
            }
        }
    }

    if (!parse_frequency_from_tokens(tokens, token_count, line_number, &ko.fixed_frequency_hz)) {
        return false;
    }

    if (!voice_definitions_add_key_override(voice_index, ko)) {
        warn(line_number, "failed to add key override — voice not defined yet");
        return false;
    }

    loaded_samples++;  // reuse counter; printed as "key overrides" in stats
    return true;
}

static bool parse_voice_line(char** tokens, uint8_t token_count, uint32_t line_number)
{
    if (token_count < 4) {
        warn(line_number, "voice line missing required fields");
        return false;
    }

    uint8_t voice_index = 0;

    if (!parse_u8(tokens[1], &voice_index) || voice_index >= MAX_VOICES) {
        warn(line_number, "invalid voice index");
        return false;
    }

    VoiceDefinition voice = {};
    voice.assigned = true;
    voice.index = voice_index;
    voice.adsr.attack_ms = 5;
    voice.adsr.decay_ms = 100;
    voice.adsr.sustain_level = 0.80f;
    voice.adsr.release_ms = 200;

    if (strcmp(tokens[2], "oscillator") == 0) {
        voice.type = VOICE_TYPE_OSCILLATOR;
        voice.oscillator = parse_oscillator_type(tokens[3]);

        if (voice.oscillator == OSC_NONE) {
            warn(line_number, "unknown oscillator type");
            return false;
        }
    }
    else if (strcmp(tokens[2], "wavetable") == 0) {
        voice.type = VOICE_TYPE_WAVETABLE;
        voice.oscillator = OSC_NONE;

        if (token_count < 4) {
            warn(line_number, "wavetable voice missing filename");
            return false;
        }

        strncpy(voice.filename, tokens[3], MAX_PATH_LENGTH - 1);
        voice.filename[MAX_PATH_LENGTH - 1] = '\0';
    }
    else if (strcmp(tokens[2], "playback") == 0) {
        voice.type = VOICE_TYPE_PLAYBACK;
        voice.oscillator = OSC_NONE;

        if (token_count >= 4 && strcmp(tokens[3], "adsr") != 0) {
            strncpy(voice.filename, tokens[3], MAX_PATH_LENGTH - 1);
            voice.filename[MAX_PATH_LENGTH - 1] = '\0';
        }
    }
    else {
        warn(line_number, "unknown voice type");
        return false;
    }

    // Scan for optional "osc2 <waveform> <detune_cents>" keyword.
    // Only meaningful for OSCILLATOR voices; silently ignored for other types.
    // osc2_type and osc2_detune default to OSC_NONE / 0 via the {} initialiser.
    if (voice.type == VOICE_TYPE_OSCILLATOR) {
        // Scan for optional "pitch <depth> <ms>" keyword.
        // depth: floating-point frequency multiplier at note-on (e.g. 4.0 = 2 oct up)
        // ms:    milliseconds to sweep from depth × freq down to note frequency
        // Not applied to OSC_NOISE oscillators (they have no fundamental pitch).
        for (uint8_t i = 0; i < token_count; i++) {
            if (strcmp(tokens[i], "pitch") != 0) continue;
            if (i + 2 >= token_count) {
                warn(line_number, "pitch requires two arguments: <depth> <ms>");
                return false;
            }
            float depth = (float)atof(tokens[i + 1]);
            int   ms    = atoi(tokens[i + 2]);
            if (depth <= 0.0f) {
                warn(line_number, "pitch depth must be > 0.0");
                return false;
            }
            if (ms < 0) {
                warn(line_number, "pitch ms must be >= 0");
                return false;
            }
            voice.pitch_env_depth = depth;
            voice.pitch_env_ms    = (uint16_t)ms;
            break;
        }
    }

    if (voice.type == VOICE_TYPE_OSCILLATOR) {
        for (uint8_t i = 0; i < token_count; i++) {
            if (strcmp(tokens[i], "osc2") != 0) continue;

            if (i + 2 >= token_count) {
                warn(line_number, "osc2 requires two arguments: <waveform> <detune_cents>");
                return false;
            }

            voice.osc2_type = parse_oscillator_type(tokens[i + 1]);
            if (voice.osc2_type == OSC_NONE) {
                warn(line_number, "unknown osc2 waveform — use sine, square, saw, or triangle");
                return false;
            }

            // strtol lets us catch non-numeric input; atoi silently returns 0.
            char* end = nullptr;
            long detune = strtol(tokens[i + 2], &end, 10);
            if (*end != '\0' || detune < -2400 || detune > 2400) {
                warn(line_number, "osc2 detune must be an integer in cents (-2400..+2400)");
                return false;
            }
            voice.osc2_detune = (int16_t)detune;
            break;
        }
    }

    for (uint8_t i = 0; i < token_count; i++) {
        if (strcmp(tokens[i], "adsr") == 0) {
            if (!parse_adsr_from_tokens(tokens, token_count, i, &voice.adsr)) {
                warn(line_number, "invalid ADSR values");
                return false;
            }

            break;
        }
    }

    parse_name_from_tokens(tokens, token_count, &voice);

    if (voice.display_name[0] == '\0') {
        snprintf(
            voice.display_name,
            MAX_VOICE_NAME_LENGTH,
            "Voice %u",
            voice.index
        );
    }

    if (!voice_definitions_set(voice)) {
        warn(line_number, "failed to store voice definition");
        return false;
    }

    loaded_voices++;
    return true;
}

static bool parse_sample_line(char** tokens, uint8_t token_count, uint32_t line_number)
{
    if (token_count < 4) {
        warn(line_number, "sample line missing required fields");
        return false;
    }

    uint8_t voice_index = 0;
    uint8_t note_index = 0;

    if (!parse_u8(tokens[1], &voice_index) || voice_index >= MAX_VOICES) {
        warn(line_number, "invalid sample voice index");
        return false;
    }

    if (!parse_u8(tokens[2], &note_index) || note_index >= MAX_SAMPLES_PER_VOICE) {
        warn(line_number, "invalid sample note index");
        return false;
    }

    const VoiceDefinition* existing = voice_definitions_get(voice_index);

    if (existing == nullptr) {
        warn(line_number, "sample references undefined voice");
        return false;
    }

    if (existing->type != VOICE_TYPE_PLAYBACK) {
        warn(line_number, "sample can only be assigned to a playback voice");
        return false;
    }

    SampleSlot slot;
    slot.assigned   = true;
    slot.note_index = note_index;
    strncpy(slot.filename, tokens[3], MAX_PATH_LENGTH - 1);
    slot.filename[MAX_PATH_LENGTH - 1] = '\0';

    if (!voice_definitions_add_sample(voice_index, slot)) {
        warn(line_number, "failed to store sample definition");
        return false;
    }

    loaded_samples++;
    return true;
}

bool voice_config_parse_line(const char* line, uint32_t line_number)
{
    if (line == nullptr) {
        return false;
    }

    parsed_lines++;

    char work[160];
    strncpy(work, line, sizeof(work) - 1);
    work[sizeof(work) - 1] = '\0';

    strip_trailing(work);

    char* text = work;
    trim_leading(&text);

    if (text[0] == '\0') {
        return true;
    }

    if (text[0] == '#') {
        return true;
    }

    char* inline_comment = strchr(text, '#');

    if (inline_comment != nullptr) {
        *inline_comment = '\0';
        strip_trailing(text);
    }

    char* tokens[16];
    uint8_t token_count = 0;

    if (!tokenize(text, tokens, 16, &token_count)) {
        warn(line_number, "too many tokens");
        return false;
    }

    if (token_count == 0) {
        return true;
    }

    if (strcmp(tokens[0], "bank") == 0) {
        if (token_count < 2) {
            warn(line_number, "bank directive missing index");
            return false;
        }
        uint8_t bank_index = 0;
        if (!parse_u8(tokens[1], &bank_index) || bank_index >= MAX_VOICE_BANKS) {
            warn(line_number, "bank index out of range");
            return false;
        }
        voice_definitions_set_bank(bank_index);
        return true;
    }

    if (strcmp(tokens[0], "voice") == 0) {
        return parse_voice_line(tokens, token_count, line_number);
    }

    if (strcmp(tokens[0], "sample") == 0) {
        return parse_sample_line(tokens, token_count, line_number);
    }

    if (strcmp(tokens[0], "key") == 0) {
        return parse_key_line(tokens, token_count, line_number);
    }

    warn(line_number, "unknown command");
    return false;
}

void voice_config_reset_stats()
{
    parsed_lines = 0;
    loaded_voices = 0;
    loaded_samples = 0;
    warning_count = 0;
    voice_definitions_set_bank(0);  // always start parsing into bank 0
}

void voice_config_print_stats()
{
    printf("Voice config parser stats:\n");
    printf("  parsed lines    = %u\n", parsed_lines);
    printf("  loaded voices   = %u\n", loaded_voices);
    printf("  loaded samples  = %u\n", loaded_samples);
    printf("  warnings        = %u\n", warning_count);
}