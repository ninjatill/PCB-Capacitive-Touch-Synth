#pragma once

#include <stdint.h>

struct RgbColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

enum StatusPatternType {
    STATUS_PATTERN_SOLID,
    STATUS_PATTERN_BLINK,
    STATUS_PATTERN_BREATHE
};

struct StatusProfile {
    RgbColor dev_color;
    RgbColor front_color;
    StatusPatternType pattern;
    uint32_t period_ms;
};

namespace StatusColors {
    constexpr RgbColor OFF   = { 0, 0, 0 };
    constexpr RgbColor RED   = { 100, 0, 0 };
    constexpr RgbColor GREEN = { 0, 80, 0 };
    constexpr RgbColor BLUE  = { 0, 0, 80 };
    constexpr RgbColor AMBER = { 100, 35, 0 };
    constexpr RgbColor CYAN  = { 0, 70, 90 };
    constexpr RgbColor WHITE = { 80, 80, 80 };
}

namespace StatusProfiles {
    constexpr StatusProfile BOOTING = {
        StatusColors::BLUE,
        StatusColors::OFF,
        STATUS_PATTERN_SOLID,
        0
    };

    constexpr StatusProfile INITIALIZING = {
        StatusColors::CYAN,
        StatusColors::OFF,
        STATUS_PATTERN_BLINK,
        500
    };

    constexpr StatusProfile OK = {
        StatusColors::GREEN,
        StatusColors::OFF,
        STATUS_PATTERN_BREATHE,
        2000
    };

    constexpr StatusProfile WARNING = {
        StatusColors::AMBER,
        StatusColors::AMBER,
        STATUS_PATTERN_BLINK,
        750
    };

    constexpr StatusProfile FAULT = {
        StatusColors::RED,
        StatusColors::RED,
        STATUS_PATTERN_BLINK,
        250
    };

    constexpr StatusProfile USB_CONNECTED = {
        StatusColors::BLUE,
        StatusColors::OFF,
        STATUS_PATTERN_SOLID,
        0
    };

    constexpr StatusProfile CHARGING = {
        StatusColors::AMBER,
        StatusColors::OFF,
        STATUS_PATTERN_BREATHE,
        1500
    };

    constexpr StatusProfile RECORDING = {
        StatusColors::RED,
        StatusColors::RED,
        STATUS_PATTERN_SOLID,
        0
    };

    constexpr StatusProfile DEV_ACTIVITY = {
        StatusColors::CYAN,
        StatusColors::OFF,
        STATUS_PATTERN_SOLID,
        0
    };
}