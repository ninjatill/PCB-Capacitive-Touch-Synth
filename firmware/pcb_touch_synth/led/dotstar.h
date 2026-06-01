#pragma once

#include "pico/stdlib.h"
#include <cstdint>

void dotstar_init();

void dotstar_set_rgb(
    uint8_t led,
    uint8_t r,
    uint8_t g,
    uint8_t b
);

void dotstar_show();