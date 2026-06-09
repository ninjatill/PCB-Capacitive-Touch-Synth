#pragma once

// ======================================================
// VOICE CONFIG PARSER
//
// Parses a plain-text voice configuration file loaded from the SD card.
// The file is read line-by-line by sample_loader and fed here one line
// at a time via voice_config_parse_line(). Results are written directly
// into the voice_definitions bank via voice_definitions_set().
//
// FILE FORMAT
//   Lines starting with '#' are comments.
//   Blank lines and inline # comments are ignored.
//   Two directive types are supported:
//
//   voice <slot> oscillator <type> [adsr <A> <D> <S> <R>] [name <display_name>]
//     Defines an oscillator voice. <slot> is 0–15.
//     <type> is one of: sine  square  saw  triangle
//     ADSR values: A=attack_ms, D=decay_ms, S=sustain(0.0–1.0), R=release_ms
//     Example:
//       voice 0 oscillator sine adsr 5 100 0.8 200 name Sine
//
//   voice <slot> wavetable <filename> [adsr ...] [name ...]
//     Loads a wavetable voice from an audio file on the SD card.
//     Example:
//       voice 3 wavetable /voices/piano.wav adsr 10 50 0.9 300 name Piano
//
//   voice <slot> playback [filename] [adsr ...] [name ...]
//     Defines a per-note sample playback voice. The optional filename is
//     a default sample used when no specific sample slot is assigned.
//     Individual note samples are added with 'sample' directives below.
//     Example:
//       voice 4 playback adsr 2 0 1.0 100 name Drums
//
//   sample <slot> <note_index> <filename>
//     Assigns one audio file to a specific keyboard note within a
//     PLAYBACK voice. <note_index> is 0–19 (matches keyboard layout).
//     The referenced voice must already be defined with a 'voice' line.
//     Example:
//       sample 4 7 /drums/kick.wav
//       sample 4 11 /drums/snare.wav
//
// TYPICAL FILE LOCATION ON SD CARD
//   /voices/voices.cfg  (path TBD when sample_loader is implemented)
//
// USAGE
//   voice_config_reset_stats();
//   // open file, read each line:
//   voice_config_parse_line(line, line_number);
//   voice_config_print_stats();
// ======================================================

#include <stdint.h>

// Parse one line from the config file. Calls voice_definitions_set()
// for valid 'voice' and 'sample' directives. Silently skips blank lines
// and comments. Returns false and increments the warning counter for
// malformed lines — parsing continues regardless.
bool voice_config_parse_line(const char* line, uint32_t line_number);

// Reset parsed_lines / loaded_voices / loaded_samples / warning counters.
// Call before starting a new config file load.
void voice_config_reset_stats();

// Print a summary of the last parse run to the debug console.
void voice_config_print_stats();