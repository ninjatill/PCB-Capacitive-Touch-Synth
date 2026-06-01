#pragma once

#include <stdint.h>

enum LedMode {
    LED_MODE_OFF,
    LED_MODE_ON,
    LED_MODE_BLINK,
    LED_MODE_BREATHE,
    LED_MODE_PULSE,
    LED_MODE_INVERSE_PULSE
};

struct LedState {
    LedMode mode;

    uint16_t brightness;
    uint32_t period_ms;

    uint32_t mode_start_ms;
    uint16_t current_pwm;
};