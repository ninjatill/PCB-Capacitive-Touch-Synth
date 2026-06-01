#include <stdio.h>

#include "pico/stdlib.h"

#include "led_animation.h"
#include "led_manager.h"
#include "led_map.h"
#include "pca9685.h"

#include "../config/i2c_addresses.h"
#include "../config/firmware_config.h"

static constexpr uint8_t LED_STATE_COUNT = 32;

static LedState led_states[LED_STATE_COUNT];

static constexpr uint32_t EXTENDED_LOW_BREATHE_MS  = 2000;
static constexpr uint32_t EXTENDED_HIGH_BREATHE_MS = 500;

static int current_octave_offset = 0;
static bool current_extended_mode = false;


// ======================================================
// INTERNAL HELPERS
// ======================================================

static int find_led_index(LedRole role, uint8_t index)
{
    for (uint8_t i = 0; i < LED_MAP_COUNT; i++) {
        if (LED_MAP[i].role == role && LED_MAP[i].index == index) {
            return i;
        }
    }

    return -1;
}

static void apply_led_pwm(uint8_t map_index, uint16_t pwm)
{
    pca9685_set_channel_brightness(
        LED_MAP[map_index].controller_addr,
        LED_MAP[map_index].channel,
        pwm
    );
}

static uint16_t compute_led_pwm(const LedState& state, uint32_t now)
{
    if (state.mode == LED_MODE_OFF) {
        return 0;
    }

    if (state.mode == LED_MODE_ON) {
        return state.brightness;
    }

    if (state.period_ms == 0) {
        return state.brightness;
    }

    uint32_t elapsed = now - state.mode_start_ms;
    uint32_t phase = elapsed % state.period_ms;
    uint32_t half = state.period_ms / 2;

    if (half == 0) {
        return state.brightness;
    }

    switch (state.mode) {
        case LED_MODE_BLINK:
            return (phase < half) ? state.brightness : 0;

        case LED_MODE_BREATHE: {
            uint32_t percent;

            if (phase < half) {
                percent = (phase * 100) / half;
            } else {
                percent = ((state.period_ms - phase) * 100) / half;
            }

            return (uint16_t)((state.brightness * percent) / 100);
        }

        case LED_MODE_PULSE:
            return (phase < half)
                ? (uint16_t)((state.brightness * (half - phase)) / half)
                : 0;

        case LED_MODE_INVERSE_PULSE:
            return (phase < half)
                ? (uint16_t)((state.brightness * phase) / half)
                : state.brightness;

        default:
            return 0;
    }
}

static void set_all_octave_leds_off()
{
    for (uint8_t i = 0; i < OCTAVE_LED_COUNT; i++) {
        led_manager_set_led(
            LED_ROLE_OCTAVE,
            i,
            LED_MODE_OFF,
            LED_GLOBAL_BRIGHTNESS,
            0
        );
    }
}

static void apply_octave_leds()
{
    set_all_octave_leds_off();

    int led_index = DEFAULT_OCTAVE_LED_INDEX + current_octave_offset;

    if (led_index >= 0 && led_index < OCTAVE_LED_COUNT) {
        led_manager_set_led(
            LED_ROLE_OCTAVE,
            (uint8_t)led_index,
            LED_MODE_ON,
            LED_GLOBAL_BRIGHTNESS,
            0
        );

        return;
    }

    if (!current_extended_mode) {
        return;
    }

    if (led_index < 0) {
        led_manager_set_led(
            LED_ROLE_OCTAVE,
            0,
            LED_MODE_BREATHE,
            LED_GLOBAL_BRIGHTNESS,
            EXTENDED_LOW_BREATHE_MS
        );

        return;
    }

    led_manager_set_led(
        LED_ROLE_OCTAVE,
        OCTAVE_LED_COUNT - 1,
        LED_MODE_BREATHE,
        LED_GLOBAL_BRIGHTNESS,
        EXTENDED_HIGH_BREATHE_MS
    );
}


// ======================================================
// PUBLIC API
// ======================================================

bool led_manager_init()
{
    printf("Initializing LED manager...\n");

    if (!pca9685_init(i2c0, I2C_ADDR_LED_1)) {
        printf("LED manager: failed to init PCA9685 LED_1\n");
        return false;
    }

    if (!pca9685_init(i2c0, I2C_ADDR_LED_2)) {
        printf("LED manager: failed to init PCA9685 LED_2\n");
        return false;
    }

    pca9685_set_pwm_freq(I2C_ADDR_LED_1, LED_PWM_FREQUENCY_HZ);
    pca9685_set_pwm_freq(I2C_ADDR_LED_2, LED_PWM_FREQUENCY_HZ);

    pca9685_all_off(I2C_ADDR_LED_1);
    pca9685_all_off(I2C_ADDR_LED_2);

    for (uint8_t i = 0; i < LED_STATE_COUNT; i++) {
        led_states[i].mode = LED_MODE_OFF;
        led_states[i].brightness = LED_GLOBAL_BRIGHTNESS;
        led_states[i].period_ms = 1000;
        led_states[i].mode_start_ms = 0;
        led_states[i].current_pwm = 0;
    }

    led_manager_set_octave(0, false);
    led_manager_set_voice(0);
    led_manager_set_recording(false);
    led_manager_set_mode(false);
    led_manager_set_midi_right(false);

    printf("LED manager initialized.\n");

    return true;
}

void led_manager_task()
{
    uint32_t now = to_ms_since_boot(get_absolute_time());

    for (uint8_t i = 0; i < LED_MAP_COUNT; i++) {
        uint16_t pwm = compute_led_pwm(led_states[i], now);

        if (pwm != led_states[i].current_pwm) {
            led_states[i].current_pwm = pwm;
            apply_led_pwm(i, pwm);
        }
    }
}

void led_manager_set_led(
    LedRole role,
    uint8_t index,
    LedMode mode,
    uint16_t brightness,
    uint32_t period_ms
)
{
    int map_index = find_led_index(role, index);

    if (map_index < 0) {
        printf("LED manager: missing LED role=%d index=%u\n", role, index);
        return;
    }

    led_states[map_index].mode = mode;
    led_states[map_index].brightness = brightness;
    led_states[map_index].period_ms = period_ms;
    led_states[map_index].mode_start_ms = to_ms_since_boot(get_absolute_time());

    uint16_t pwm = compute_led_pwm(
        led_states[map_index],
        led_states[map_index].mode_start_ms
    );

    led_states[map_index].current_pwm = pwm;
    apply_led_pwm((uint8_t)map_index, pwm);
}

void led_manager_set_note(uint8_t note_index, bool on)
{
    if (note_index >= 20) {
        return;
    }

    led_manager_set_led(
        LED_ROLE_NOTE,
        note_index,
        on ? LED_MODE_ON : LED_MODE_OFF,
        LED_GLOBAL_BRIGHTNESS,
        0
    );
}

void led_manager_set_octave(int octave_offset, bool extended_mode)
{
    current_octave_offset = octave_offset;
    current_extended_mode = extended_mode;

    apply_octave_leds();
}

void led_manager_set_voice(uint8_t voice)
{
    led_manager_set_led(
        LED_ROLE_VOICE,
        VOICE_LED_1,
        (voice & 0x01) ? LED_MODE_ON : LED_MODE_OFF,
        LED_GLOBAL_BRIGHTNESS,
        0
    );

    led_manager_set_led(
        LED_ROLE_VOICE,
        VOICE_LED_2,
        (voice & 0x02) ? LED_MODE_ON : LED_MODE_OFF,
        LED_GLOBAL_BRIGHTNESS,
        0
    );

    led_manager_set_led(
        LED_ROLE_VOICE,
        VOICE_LED_4,
        (voice & 0x04) ? LED_MODE_ON : LED_MODE_OFF,
        LED_GLOBAL_BRIGHTNESS,
        0
    );

    led_manager_set_led(
        LED_ROLE_VOICE,
        VOICE_LED_8,
        (voice & 0x08) ? LED_MODE_ON : LED_MODE_OFF,
        LED_GLOBAL_BRIGHTNESS,
        0
    );
}

void led_manager_set_recording(bool active)
{
    led_manager_set_led(
        LED_ROLE_STATUS,
        STATUS_LED_RECORD,
        active ? LED_MODE_ON : LED_MODE_OFF,
        LED_GLOBAL_BRIGHTNESS,
        0
    );
}

void led_manager_set_mode(bool midi_mode_active)
{
    led_manager_set_led(
        LED_ROLE_STATUS,
        STATUS_LED_MODE,
        midi_mode_active ? LED_MODE_ON : LED_MODE_OFF,
        LED_GLOBAL_BRIGHTNESS,
        0
    );
}

void led_manager_set_midi_right(bool right_side)
{
    led_manager_set_led(
        LED_ROLE_STATUS,
        STATUS_LED_MIDI_LR,
        right_side ? LED_MODE_ON : LED_MODE_OFF,
        LED_GLOBAL_BRIGHTNESS,
        0
    );
}