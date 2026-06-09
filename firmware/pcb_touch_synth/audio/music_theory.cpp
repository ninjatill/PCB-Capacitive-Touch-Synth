

#include <math.h>
#include <stdio.h>

#include "music_theory.h"

static constexpr float A4_FREQUENCY_HZ = 440.0f;
static constexpr int A4_MIDI_NOTE = 69;

static const char* NOTE_NAMES[] = {
    "C",
    "C#",
    "D",
    "D#",
    "E",
    "F",
    "F#",
    "G",
    "G#",
    "A",
    "A#",
    "B"
};

float midi_note_to_frequency(uint8_t midi_note)
{
    return A4_FREQUENCY_HZ *
           powf(
               2.0f,
               ((float)midi_note - (float)A4_MIDI_NOTE) / 12.0f
           );
}

const char* midi_note_name(uint8_t midi_note)
{
    return NOTE_NAMES[midi_note % 12];
}

int clamp_midi_note(int midi_note)
{
    if (midi_note < 0) {
        return 0;
    }

    if (midi_note > 127) {
        return 127;
    }

    return midi_note;
}