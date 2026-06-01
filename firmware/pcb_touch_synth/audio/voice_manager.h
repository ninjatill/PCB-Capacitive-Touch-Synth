#pragma once

#include <stdint.h>

bool voice_manager_init();

uint8_t voice_manager_get_current();
void voice_manager_set_current(uint8_t voice);
uint8_t voice_manager_next();

bool voice_manager_is_recorded_voice(uint8_t voice);