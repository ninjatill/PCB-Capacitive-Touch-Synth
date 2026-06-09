#pragma once

// ======================================================
// AUDIO COMMAND QUEUE — INTER-CORE MESSAGE PASSING
//
// The only safe way to send instructions from core 0 to core 1.
// Implemented as a lock-free circular FIFO using interrupt masking
// (save_and_disable_interrupts) to make push and pop atomic on the
// single-issue Cortex-M0+ without needing a mutex or spinlock.
//
// PRODUCER: core 0 (audio_manager, control_manager)
//   Calls audio_commands_push() — typically from touch events or
//   recording start/stop requests.
//
// CONSUMER: core 1 (audio_core1_main render loop)
//   Calls audio_commands_pop() each render iteration. Commands are
//   processed in order. A full queue drops the incoming command and
//   logs a warning — the queue size (32) should be far larger than
//   the maximum burst rate from key presses.
//
// RULES
//   - Only push from core 0. Only pop from core 1.
//   - Never push from within an IRQ handler; audio_manager is the
//     intended sole producer.
//   - Do not add fields to AudioCommand that require heap allocation
//     or pointer lifetimes that span cores.
// ======================================================

#include <stdint.h>

enum AudioCommandType {
    AUDIO_CMD_NONE,
    AUDIO_CMD_NOTE_ON,          // note_index, midi_note, voice fields valid
    AUDIO_CMD_NOTE_OFF,         // note_index, midi_note fields valid
    AUDIO_CMD_START_RECORDING,  // no payload; switches core 1 to RECORDING mode
    AUDIO_CMD_STOP_RECORDING,   // no payload; returns core 1 to IDLE
    AUDIO_CMD_SET_IDLE          // no payload; force IDLE regardless of current mode
};

// Payload is a fixed-size POD struct so it can be copied safely across cores.
struct AudioCommand {
    AudioCommandType type;
    uint8_t note_index; // Physical keyboard note index (0–19)
    uint8_t midi_note;  // MIDI note number (0–127; e.g. C4 = 60)
    uint8_t voice;      // Voice slot index (0–15)
};

// Push a command onto the queue. Returns false and logs a warning if full.
// Call from core 0 only.
bool audio_commands_push(const AudioCommand& command);

// Pop the next command from the queue. Returns false if empty.
// Call from core 1 only (audio_core1_main render loop).
bool audio_commands_pop(AudioCommand* command);