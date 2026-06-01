#pragma once

#include <stdint.h>

bool audio_manager_init();
void audio_manager_task();

void audio_manager_set_volume(int volume);
int audio_manager_get_volume();

void audio_manager_volume_up();
void audio_manager_volume_down();

void audio_manager_note_on(uint8_t note_index, uint8_t midi_note);
void audio_manager_note_off(uint8_t note_index, uint8_t midi_note);