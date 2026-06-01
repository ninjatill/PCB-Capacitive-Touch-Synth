#pragma once

#include <stdint.h>

bool rotary_encoder_init();

int32_t rotary_encoder_get_delta();
void rotary_encoder_clear_delta();