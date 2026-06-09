#include <stdio.h>
#include <stdint.h>

#include "pico/stdlib.h"

#include "status_led.h"
#include "status_profiles.h"

#include "../led/dotstar.h"
#include "../config/firmware_config.h"

static bool status_led_ready = false;

static SystemStatus current_status = SYSTEM_STATUS_BOOTING;

static uint32_t last_update_ms = 0;
static bool blink_state = false;

static constexpr uint8_t DEV_LED = 0;
static constexpr uint8_t FRONT_LED = 1;

static RgbColor scale_color(RgbColor color, uint8_t percent)
{
    RgbColor scaled;

    scaled.r = (uint8_t)((color.r * percent) / 100);
    scaled.g = (uint8_t)((color.g * percent) / 100);
    scaled.b = (uint8_t)((color.b * percent) / 100);

    return scaled;
}

static const StatusProfile& get_profile(SystemStatus status)
{
    switch (status) {
        case SYSTEM_STATUS_BOOTING:
            return StatusProfiles::BOOTING;

        case SYSTEM_STATUS_INITIALIZING:
            return StatusProfiles::INITIALIZING;

        case SYSTEM_STATUS_OK:
            return StatusProfiles::OK;

        case SYSTEM_STATUS_WARNING:
            return StatusProfiles::WARNING;

        case SYSTEM_STATUS_FAULT:
            return StatusProfiles::FAULT;

        case SYSTEM_STATUS_USB_CONNECTED:
            return StatusProfiles::USB_CONNECTED;

        case SYSTEM_STATUS_CHARGING:
            return StatusProfiles::CHARGING;

        case SYSTEM_STATUS_RECORDING:
            return StatusProfiles::RECORDING;

        case SYSTEM_STATUS_DEV_ACTIVITY:
            return StatusProfiles::DEV_ACTIVITY;

        case SYSTEM_STATUS_STARTING_AUDIO:
            return StatusProfiles::STARTING_AUDIO;

        case SYSTEM_STATUS_STARTING_TOUCH:
            return StatusProfiles::STARTING_TOUCH;

        case SYSTEM_STATUS_MIDI_CONNECTING:
            return StatusProfiles::MIDI_CONNECTING;

        case SYSTEM_STATUS_MIDI_ACTIVE:
            return StatusProfiles::MIDI_ACTIVE;

        default:
            return StatusProfiles::FAULT;
    }
}

static void show_status_leds(RgbColor dev, RgbColor front)
{
    dotstar_set_rgb(DEV_LED, dev.r, dev.g, dev.b);

    if (STATUS_FRONT_LED_MIRRORS_DEV_LED) {
        dotstar_set_rgb(FRONT_LED, dev.r, dev.g, dev.b);
    } else {
        dotstar_set_rgb(FRONT_LED, front.r, front.g, front.b);
    }

    dotstar_show();
}

bool status_led_initialized()
{
    return status_led_ready;
}

bool status_led_init()
{
    printf("Initializing status LEDs...\n");

    dotstar_init();

    current_status = SYSTEM_STATUS_BOOTING;
    last_update_ms = to_ms_since_boot(get_absolute_time());
    blink_state = false;
    status_led_ready = true;

    status_led_set(SYSTEM_STATUS_BOOTING);

    printf("Status LEDs initialized.\n");

    return true;
}

void status_led_set(SystemStatus status)
{
    current_status = status;
    last_update_ms = 0;
    blink_state = false;

    printf("System status changed: %d\n", status);

    status_led_task();
}

SystemStatus status_led_get()
{
    return current_status;
}

void status_led_task()
{
    if (!status_led_ready) {
        return;
    }

    const StatusProfile& profile = get_profile(current_status);
    uint32_t now = to_ms_since_boot(get_absolute_time());

    switch (profile.pattern) {
        case STATUS_PATTERN_SOLID:
            show_status_leds(profile.dev_color, profile.front_color);
            break;

        case STATUS_PATTERN_BLINK:
            if ((now - last_update_ms) >= profile.period_ms) {
                last_update_ms = now;
                blink_state = !blink_state;
            }

            if (blink_state) {
                show_status_leds(profile.dev_color, profile.front_color);
            } else {
                show_status_leds(StatusColors::OFF, StatusColors::OFF);
            }

            break;

        case STATUS_PATTERN_BREATHE: {
            uint32_t phase = now % profile.period_ms;
            uint32_t half_period = profile.period_ms / 2;

            uint8_t percent;

            if (phase < half_period) {
                percent = (uint8_t)((phase * 100) / half_period);
            } else {
                percent = (uint8_t)(((profile.period_ms - phase) * 100) / half_period);
            }

            // Keep it from fully disappearing.
            if (percent < 10) {
                percent = 10;
            }

            show_status_leds(
                scale_color(profile.dev_color, percent),
                scale_color(profile.front_color, percent)
            );

            break;
        }

        default:
            show_status_leds(StatusColors::OFF, StatusColors::OFF);
            break;
    }
}