#pragma once

#include <stdint.h>

enum AudioCoreMode {
    AUDIO_CORE_MODE_IDLE,
    AUDIO_CORE_MODE_PLAYBACK,
    AUDIO_CORE_MODE_RECORDING
};

bool audio_core_init();
void audio_core_start_on_core1();

void audio_core_set_mode(AudioCoreMode mode);

void audio_core_note_on(uint8_t note_index, uint8_t midi_note, uint8_t voice);
void audio_core_note_off(uint8_t note_index, uint8_t midi_note);