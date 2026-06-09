#pragma once

// ======================================================
// SYSEX FILE TRANSFER — RP2040 IMPLEMENTATION
//
// Transfers a WAV file from SD card to the host RP400 over USB MIDI
// using the PCB Touch Synth SysEx protocol (sysex_protocol.h).
//
// CORE ASSIGNMENT
//   Core 0 (system_tasks loop):
//     - Receives FILE_REQUEST SysEx via midi_manager_receive_task()
//     - Calls sysex_transfer_start() to begin the transfer
//     - Calls sysex_transfer_drain() each loop to push queued SysEx
//       messages into the TinyUSB MIDI TX FIFO
//
//   Core 1 (audio_core1_main loop, idle in MIDI mode):
//     - Calls sysex_transfer_task() each iteration
//     - Reads SD card in 63-byte chunks, 7-bit encodes, builds
//       DATA_BLOCK messages, pushes to the inter-core ring buffer
//     - After last block: calculates CRC32 and pushes TRANSFER_DONE
//
// RING BUFFER
//   A fixed-size circular buffer of pre-allocated SysEx message slots
//   decouples the SD card reader (core 1) from the USB sender (core 0).
//   Protected by an RP2040 hardware spin lock.
// ======================================================

#include <stdint.h>
#include <stdbool.h>
#include "sysex_protocol.h"

// ---- Lifecycle ----

// Initialise the ring buffer and transfer state. Call from system_init().
void sysex_transfer_init();

// Begin a file transfer in response to a FILE_REQUEST from the host.
// Sends the FILE_HEADER SysEx immediately (file size + block count).
// Signals core 1 to start producing DATA_BLOCK messages.
// Safe to call from core 0.
bool sysex_transfer_start(const char* filepath);

// Abort an in-progress transfer.  Sends TRANSFER_ABORT to the host.
void sysex_transfer_abort();

// True while a transfer is in progress.
bool sysex_transfer_is_active();

// True while core 1 has the SD card file open and is actively reading.
// Core 0 must not touch SPI0 (DotStar) while this returns true because
// both cores share SPI0 and the 74AHCT2G125 only provides electrical
// isolation — it does not prevent simultaneous register access on the SPI
// peripheral itself.  Set by core 1 on f_open(); cleared on f_close() or abort.
bool sysex_core1_spi_busy();

// ---- Core 0 role: drain ring buffer to TinyUSB each loop ----

// Pop queued SysEx messages from the ring buffer and write them to
// the TinyUSB MIDI stream.  Call once per system_tasks() iteration,
// after tud_task() has had a chance to flush the TX FIFO.
void sysex_transfer_drain();

// ---- Core 1 role: produce DATA_BLOCK messages ----

// Called from audio_core1_main() when sysex_transfer_is_active().
// Reads the next chunk from SD card, 7-bit encodes it, builds a
// DATA_BLOCK SysEx message, pushes to the ring buffer, then returns.
// When the last block is pushed, also queues the TRANSFER_DONE message.
// Non-blocking: does nothing if the ring buffer is full this call.
void sysex_transfer_task();

// ---- Utility (exposed for use in FILE_HEADER build) ----

// Encode 'src_len' binary bytes from 'src' into 7-bit MIDI-safe bytes
// at 'dst'.  dst must be at least ceil(src_len * 8 / 7) bytes.
// Returns the number of bytes written to dst.
size_t sysex_encode_7bit(const uint8_t* src, size_t src_len, uint8_t* dst);

// Decode 7-bit encoded MIDI bytes back to binary.
// Returns the number of binary bytes recovered.
size_t sysex_decode_7bit(const uint8_t* src, size_t src_len, uint8_t* dst);
