#pragma once

// ======================================================
// MUSIC THEORY — MIDI / FREQUENCY UTILITIES
//
// Standard MIDI note numbering: 0–127.
// A4 (concert pitch) = MIDI note 69 = 440.0 Hz.
// Each semitone is a factor of 2^(1/12) ≈ 1.05946.
// Formula: freq = 440.0 × 2^((note − 69) / 12)
//
// Note names are pitch class only (C, C#, D … B).
// No octave number is returned — add (note / 12 − 1) if needed.
// ======================================================

#include <stdint.h>

// Convert a MIDI note number to its fundamental frequency in Hz.
float midi_note_to_frequency(uint8_t midi_note);

// Return the pitch-class name ("C", "C#", "D" … "B") for a MIDI note.
const char* midi_note_name(uint8_t midi_note);

// Clamp an integer (possibly negative or > 127) to the valid MIDI range 0–127.
int clamp_midi_note(int midi_note);