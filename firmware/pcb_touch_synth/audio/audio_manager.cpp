#include <stdio.h>

#include "audio_commands.h"
#include "audio_manager.h"
#include "synth_engine.h"
#include "tlv320dac3100.h"
#include "voice_manager.h"

#include "../config/i2c_addresses.h"
#include "../input/rotary_encoder.h"

static constexpr int AUDIO_VOLUME_MIN = 0;
static constexpr int AUDIO_VOLUME_MAX = 100;
static constexpr int AUDIO_VOLUME_DEFAULT = 35;
static constexpr int AUDIO_VOLUME_STEP = 2;

static bool audio_initialized  = false;
static int  current_volume     = AUDIO_VOLUME_DEFAULT;
static bool headphones_inserted = false;
static bool speaker_allowed    = true;  // false when on USB 100mA or 500mA

bool audio_manager_initialized()
{
    return audio_initialized;
}

bool audio_manager_headphones_inserted()
{
    return headphones_inserted;
}

static float volume_to_db(int volume)
{
    if (volume <= 0) {
        return -78.0f;
    }

    if (volume >= 100) {
        return 0.0f;
    }

    // Simple linear mapping for now:
    // 0   -> -78 dB
    // 100 ->   0 dB
    return -78.0f + ((float)volume * 78.0f / 100.0f);
}

static void audio_manager_apply_volume()
{
    float db = volume_to_db(current_volume);

    printf("AUDIO: applying volume=%d db=%.1f\n", current_volume, db);

    tlv320dac3100_set_headphone_volume_db(db);
    tlv320dac3100_set_speaker_volume_db(db);
}

bool audio_manager_init()
{
    printf("Initializing audio manager...\n");

    if (!tlv320dac3100_init(i2c1, I2C_ADDR_AUDIO_DAC)) {
        printf("Audio manager: TLV320DAC3100 init failed.\n");
        return false;
    }

    if (!rotary_encoder_init()) {
        printf("Audio manager: rotary encoder init failed.\n");
        return false;
    }

    // tlv320dac3100_init() already enables headphone detection and powers up
    // the speaker as the default output. Read initial state so task() has a baseline.
    tlv320dac3100_headphone_inserted(&headphones_inserted);

    if (headphones_inserted) {
        tlv320dac3100_enable_headphones();
    }

    audio_manager_apply_volume();

    audio_initialized = true;

    printf("Audio manager initialized. volume=%d headphones=%s\n",
           current_volume,
           headphones_inserted ? "inserted" : "not inserted");

    return true;
}

void audio_manager_task()
{
    int32_t delta = rotary_encoder_get_delta();

    if (delta != 0) {
        printf("AUDIO: encoder delta=%ld\n", delta);

        if (delta > 0) {
            for (int32_t i = 0; i < delta; i++) {
                audio_manager_volume_up();
            }
        }
        else {
            for (int32_t i = 0; i < (-delta); i++) {
                audio_manager_volume_down();
            }
        }

        // If notes are currently playing the user hears the volume change
        // directly in the audio — no extra feedback needed.
        // If silent, play a short chirp so they can hear the current level.
        if (!synth_engine_notes_active()) {
            tlv320dac3100_play_beep_1khz();
        }
    }

    bool inserted = false;

    if (tlv320dac3100_headphone_inserted(&inserted)) {
        if (inserted != headphones_inserted) {
            headphones_inserted = inserted;

            printf("AUDIO: headphones %s\n",
                   headphones_inserted ? "inserted" : "removed");

            if (headphones_inserted) {
                tlv320dac3100_enable_headphones();
            } else if (speaker_allowed) {
                tlv320dac3100_enable_speaker();
            } else {
                // Headphones removed but speaker not allowed (limited USB power).
                tlv320dac3100_mute_dac(true);
                printf("AUDIO: headphones removed — speaker suppressed (limited USB power).\n");
            }
        }
    }
}

void audio_manager_set_volume(int volume)
{
    if (volume < AUDIO_VOLUME_MIN) {
        volume = AUDIO_VOLUME_MIN;
    }

    if (volume > AUDIO_VOLUME_MAX) {
        volume = AUDIO_VOLUME_MAX;
    }

    if (volume == current_volume) {
        return;
    }

    current_volume = volume;
    audio_manager_apply_volume();
}

int audio_manager_get_volume()
{
    return current_volume;
}

void audio_manager_volume_up()
{
    audio_manager_set_volume(current_volume + AUDIO_VOLUME_STEP);
}

void audio_manager_volume_down()
{
    audio_manager_set_volume(current_volume - AUDIO_VOLUME_STEP);
}

void audio_manager_note_on(uint8_t note_index, uint8_t midi_note)
{
    uint8_t voice = voice_manager_get_current();

    printf("AUDIO: queue note on index=%u midi=%u voice=%u volume=%d\n",
           note_index,
           midi_note,
           voice,
           current_volume);

    AudioCommand command = {
        AUDIO_CMD_NOTE_ON,
        note_index,
        midi_note,
        voice
    };

    audio_commands_push(command);
}

void audio_manager_note_off(uint8_t note_index, uint8_t midi_note)
{
    uint8_t voice = voice_manager_get_current();

    printf("AUDIO: queue note off index=%u midi=%u voice=%u\n",
           note_index,
           midi_note,
           voice);

    AudioCommand command = {
        AUDIO_CMD_NOTE_OFF,
        note_index,
        midi_note,
        voice
    };

    audio_commands_push(command);
}

void audio_manager_apply_power_mode(bool allow_speaker)
{
    if (speaker_allowed == allow_speaker) return;

    speaker_allowed = allow_speaker;

    printf("AUDIO: power mode update — speaker %s\n",
           speaker_allowed ? "allowed" : "suppressed");

    if (!audio_initialized) return;

    if (speaker_allowed) {
        // Power mode upgraded to full — restore normal output routing.
        if (headphones_inserted) {
            tlv320dac3100_enable_headphones();
        } else {
            tlv320dac3100_enable_speaker();
        }
    } else {
        // Power mode degraded — disable speaker, route to headphones if present.
        if (headphones_inserted) {
            tlv320dac3100_enable_headphones();
        } else {
            tlv320dac3100_mute_dac(true);
        }
    }
}