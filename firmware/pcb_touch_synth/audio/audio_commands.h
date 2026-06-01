#pragma once

#include <stdint.h>

enum AudioCommandType {
    AUDIO_CMD_NONE,
    AUDIO_CMD_NOTE_ON,
    AUDIO_CMD_NOTE_OFF,
    AUDIO_CMD_START_RECORDING,
    AUDIO_CMD_STOP_RECORDING,
    AUDIO_CMD_SET_IDLE
};

struct AudioCommand {
    AudioCommandType type;
    uint8_t note_index;
    uint8_t midi_note;
    uint8_t voice;
};

bool audio_commands_push(const AudioCommand& command);
bool audio_commands_pop(AudioCommand* command);