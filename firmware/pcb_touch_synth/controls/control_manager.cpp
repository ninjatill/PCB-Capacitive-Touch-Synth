#include <stdio.h>

#include "pico/stdlib.h"

#include "../audio/audio_manager.h"
#include "../audio/voice_manager.h"
#include "control_manager.h"
#include "../config/firmware_config.h"

static int current_octave_offset = 0;
static bool extended_octaves_enabled = false;

static uint32_t octave_press_time = 0;
static uint32_t voice_press_time = 0;
static uint32_t mode_press_time = 0;
static uint32_t record_press_time = 0;

bool control_manager_init()
{
    printf("Initializing control manager...\n");

    printf("Default voice: %u\n", voice_manager_get_current());
    current_octave_offset = 0;
    extended_octaves_enabled = false;

    printf("Default octave offset: %d\n", current_octave_offset);

    return true;
}

void control_manager_note_pressed(uint8_t note_index, uint8_t midi_note)
{
    int shifted_note = midi_note + current_octave_offset * 12;
    uint8_t voice = voice_manager_get_current();

    printf("CONTROL: note on index=%u midi=%u shifted=%d voice=%d\n",
           note_index,
           midi_note,
           shifted_note,
           voice);

    led_manager_set_note(note_index, true);

    audio_manager_note_on(
        note_index,
        shifted_note
    );
}

void control_manager_note_released(uint8_t note_index, uint8_t midi_note)
{
    int shifted_note = midi_note + current_octave_offset * 12;

    printf("CONTROL: note off index=%u midi=%u shifted=%d\n",
           note_index,
           midi_note,
           shifted_note);

    led_manager_set_note(note_index, false);

    audio_manager_note_off(
        note_index,
        shifted_note
    );
}

void control_manager_control_pressed(TouchControl control)
{
    uint32_t now = to_ms_since_boot(get_absolute_time());

    switch (control) {
        case TOUCH_CONTROL_OCTAVE:
            octave_press_time = now;
            printf("CONTROL: octave pressed\n");
            break;

        case TOUCH_CONTROL_VOICE:
            voice_press_time = now;
            printf("CONTROL: voice pressed\n");
            break;

        case TOUCH_CONTROL_MODE:
            mode_press_time = now;
            printf("CONTROL: mode pressed\n");
            break;

        case TOUCH_CONTROL_RECORD:
            record_press_time = now;
            printf("CONTROL: record pressed\n");
            break;

        default:
            break;
    }
}

void control_manager_control_released(TouchControl control)
{
    uint32_t now = to_ms_since_boot(get_absolute_time());

    switch (control) {
        case TOUCH_CONTROL_OCTAVE: {
            uint32_t held_ms = now - octave_press_time;

            if (held_ms >= OCTAVE_EXTENDED_HOLD_MS) {
                extended_octaves_enabled = !extended_octaves_enabled;
                printf("CONTROL: extended octaves %s\n",
                    extended_octaves_enabled ? "enabled" : "disabled");
            }
            else {
                int max_offset = extended_octaves_enabled ? 4 : 2;
                int min_offset = extended_octaves_enabled ? -4 : -2;

                current_octave_offset++;

                if (current_octave_offset > max_offset) {
                    current_octave_offset = min_offset;
                }

                printf("CONTROL: octave offset now %d\n", current_octave_offset);
            }

            led_manager_set_octave(current_octave_offset, extended_octaves_enabled);

            break;
        }

        case TOUCH_CONTROL_VOICE: {
                uint8_t voice = voice_manager_next();

                printf("CONTROL: voice now %u\n", voice);

                led_manager_set_voice(voice);

                break;
            }
        case TOUCH_CONTROL_MODE:
            printf("CONTROL: mode released\n");
            break;

        case TOUCH_CONTROL_RECORD:
            printf("CONTROL: record released\n");
            break;

        default:
            break;
    }
}

void control_manager_task()
{
    // Future:
    // - detect long-press while held
    // - record countdown/blinks
    // - mode-setting state machine
}