#include <stdio.h>

#include "pico/stdlib.h"

#include "../audio/audio_manager.h"
#include "../audio/midi_manager.h"
#include "../audio/voice_definitions.h"
#include "../audio/voice_manager.h"
#include "control_manager.h"
#include "../config/firmware_config.h"

// ---- Octave state ----
static int  current_octave_offset    = 0;
static bool extended_octaves_enabled = false;

// ---- MIDI mode arm state ----
// Tracks whether the MODE button has already signalled the 10s threshold
// while still held (so we only fire the feedback once per press).
static bool midi_arm_indicated = false;

// ---- Voice bank state ----
// When extended_voice_mode is true the VOICE button cycles banks instead of
// voices.  The voice LEDs switch to a blinking binary pattern showing the
// current bank number.  A second 5-second hold exits bank mode and returns
// the LEDs to normal solid binary voice display.
// Bank selection is unavailable when only bank 0 is configured.
static bool extended_voice_mode = false;

// ---- Button press timestamps (recorded on press, used on release) ----
// Hold duration = (release_time - press_time).  All four buttons use the
// same pattern: short release = tap action, long release = hold action.
static uint32_t octave_press_time = 0;
static uint32_t voice_press_time  = 0;
static uint32_t mode_press_time   = 0;
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
    if (shifted_note < 0)   shifted_note = 0;
    if (shifted_note > 127) shifted_note = 127;

    led_manager_set_note(note_index, true);

    if (midi_manager_is_enabled()) {
        // MIDI mode: forward to host PC as MIDI Note On.
        // Velocity 127 = full; local synth is silent.
        printf("CONTROL: MIDI note on index=%u note=%d ch=%u\n",
               note_index, shifted_note, midi_manager_get_channel());
        midi_manager_note_on((uint8_t)shifted_note, 127);
    } else {
        // Local synthesis mode: send to the on-board synth engine.
        printf("CONTROL: note on index=%u midi=%u shifted=%d voice=%u\n",
               note_index, midi_note, shifted_note, voice_manager_get_current());
        audio_manager_note_on(note_index, (uint8_t)shifted_note);
    }
}

void control_manager_note_released(uint8_t note_index, uint8_t midi_note)
{
    int shifted_note = midi_note + current_octave_offset * 12;
    if (shifted_note < 0)   shifted_note = 0;
    if (shifted_note > 127) shifted_note = 127;

    led_manager_set_note(note_index, false);

    if (midi_manager_is_enabled()) {
        printf("CONTROL: MIDI note off index=%u note=%d\n",
               note_index, shifted_note);
        midi_manager_note_off((uint8_t)shifted_note);
    } else {
        printf("CONTROL: note off index=%u midi=%u shifted=%d\n",
               note_index, midi_note, shifted_note);
        audio_manager_note_off(note_index, (uint8_t)shifted_note);
    }
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
            mode_press_time    = now;
            midi_arm_indicated = false;
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
            uint32_t held_ms = now - voice_press_time;

            if (held_ms >= VOICE_BANK_HOLD_MS) {
                // Long hold: toggle bank selection mode, identical pattern to
                // octave extended mode.  Guard: do nothing if only bank 0 exists.
                uint8_t configured = voice_definitions_configured_bank_count();
                if (configured <= 1) {
                    printf("CONTROL: bank selection unavailable — only bank 0 configured\n");
                    break;
                }

                extended_voice_mode = !extended_voice_mode;
                printf("CONTROL: voice bank selection %s\n",
                       extended_voice_mode ? "enabled" : "disabled");

                if (extended_voice_mode) {
                    led_manager_set_voice_bank_select(voice_manager_get_bank());
                } else {
                    led_manager_set_voice(voice_manager_get_current());
                }
            } else if (extended_voice_mode) {
                // Short press while in bank selection mode: advance to next bank.
                uint8_t bank = voice_manager_next_bank();
                printf("CONTROL: voice bank now %u\n", bank);
                led_manager_set_voice_bank_select(bank);
            } else {
                // Normal short press: advance to next assigned voice in current bank.
                uint8_t voice = voice_manager_next();
                printf("CONTROL: voice now %u\n", voice);
                led_manager_set_voice(voice);
            }
            break;
        }
        case TOUCH_CONTROL_MODE: {
            uint32_t held_ms = now - mode_press_time;

            if (held_ms >= MIDI_MODE_ENABLE_HOLD_MS) {
                // 10-second hold: attempt USB MIDI enumeration.
                // The MODE LED was already blinking since the threshold was
                // reached (feedback in control_manager_task).
                printf("CONTROL: MODE 10s hold — attempting MIDI enable\n");
                midi_manager_try_enable();

            } else if (held_ms >= MODE_SETTINGS_HOLD_MS) {
                // 7-second hold: settings (future use).
                printf("CONTROL: MODE settings hold (%u ms) — not yet implemented\n", held_ms);

            } else if (held_ms >= MIDI_LR_TOGGLE_HOLD_MS) {
                // 3-second hold: toggle MIDI L/R channel.
                midi_manager_toggle_lr_channel();
                printf("CONTROL: MIDI L/R → %s\n",
                       midi_manager_is_right_channel() ? "RIGHT (ch2)" : "LEFT (ch1)");

            } else {
                printf("CONTROL: MODE tap (%u ms)\n", held_ms);
            }

            midi_arm_indicated = false;  // reset for next press
            break;
        }

        case TOUCH_CONTROL_RECORD: {
            uint32_t held_ms = now - record_press_time;

            if (held_ms >= RECORD_HOLD_MS) {
                // Long hold (3s): arm recording.
                // TODO: push AUDIO_CMD_START_RECORDING and start arm-blink sequence.
                printf("CONTROL: recording armed (hold=%u ms) — wiring pending\n", held_ms);
            } else {
                // Short press while recording is active: stop recording.
                // TODO: push AUDIO_CMD_STOP_RECORDING.
                printf("CONTROL: record tap (%u ms)\n", held_ms);
            }
            break;
        }

        default:
            break;
    }
}

int control_manager_get_octave_offset()
{
    return current_octave_offset;
}

void control_manager_task()
{
    uint32_t now = to_ms_since_boot(get_absolute_time());

    // While-held feedback: MODE button at the 10-second MIDI arm threshold.
    // Fire once per press (midi_arm_indicated prevents repeated triggers).
    // This tells the user "you've held long enough — release to engage MIDI"
    // before they have to let go and guess whether it registered.
    if (mode_press_time > 0 && !midi_arm_indicated) {
        uint32_t held = now - mode_press_time;
        if (held >= MIDI_MODE_ENABLE_HOLD_MS) {
            midi_arm_indicated = true;

            // Rapid blink on MODE LED: "threshold reached, release now"
            led_manager_set_led(LED_ROLE_STATUS, STATUS_LED_MODE,
                                LED_MODE_BLINK, LED_GLOBAL_BRIGHTNESS, 150);

            printf("CONTROL: MODE 10s threshold reached — release to engage MIDI.\n");
        }
    }
}