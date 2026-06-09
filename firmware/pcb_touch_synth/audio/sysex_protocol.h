#pragma once

// ======================================================
// PCB TOUCH SYNTH — SYSEX FILE TRANSFER PROTOCOL
//
// This header defines the complete bidirectional SysEx protocol used
// to transfer voice recording WAV files from the touch synth keyboard
// to the host Raspberry Pi 400 over USB MIDI.
//
// All SysEx messages use the non-commercial manufacturer ID 0x7D
// (reserved for experimental/educational use, no registration needed).
//
// ══════════════════════════════════════════════════════
// HOST-SIDE IMPLEMENTATION GUIDE (Python / RP400)
// ══════════════════════════════════════════════════════
//
// Required libraries:
//   pip install mido python-rtmidi pyfluidsynth sf2utils
//
// Python pseudocode for the full transfer flow:
//
//   import mido, struct, zlib
//
//   MANUFACTURER = 0x7D
//   DEVICE_ID    = 0x00
//
//   def on_sysex(msg):
//       data = msg.data           # list of ints, F0/F7 stripped by mido
//       if data[0] != MANUFACTURER or data[1] != DEVICE_ID:
//           return
//       msg_type = data[2]
//
//       if msg_type == 0x01:      # SAMPLE_READY
//           filename = bytes(data[3:]).decode('ascii')
//           port.send(mido.Message('sysex', data=[MANUFACTURER, DEVICE_ID, 0x02]))
//           # → sends FILE_REQUEST back to keyboard
//
//       elif msg_type == 0x03:    # FILE_HEADER
//           file_size = decode_7bit_uint32(data[3:8])
//           block_count = (data[8] << 7) | data[9]
//           blocks.clear()
//
//       elif msg_type == 0x04:    # DATA_BLOCK
//           seq = (data[3] << 7) | data[4]
//           raw = decode_7bit(data[5:-1])   # last byte is block CRC8
//           crc8_rx = data[-1]
//           if crc8_compute(raw) == crc8_rx:
//               blocks[seq] = raw
//
//       elif msg_type == 0x05:    # TRANSFER_DONE
//           crc32_rx = decode_7bit_uint32(data[3:8])
//           wav = b''.join(blocks[i] for i in sorted(blocks))
//           if zlib.crc32(wav) & 0xFFFFFFFF == crc32_rx:
//               save_and_load(wav)
//           else:
//               print("CRC32 mismatch — transfer corrupted")
//
//       elif msg_type == 0x07:    # TRANSFER_ABORT
//           blocks.clear()
//
//   def decode_7bit(sysex_bytes):
//       # Inverse of the 7-bit encoding below.
//       # 8 SysEx bytes → 7 binary bytes.
//       result = bytearray()
//       for i in range(0, len(sysex_bytes), 8):
//           group = sysex_bytes[i:i+8]
//           msb   = group[0]
//           for j in range(1, min(8, len(group))):
//               result.append(group[j] | (((msb >> (j-1)) & 1) << 7))
//       return bytes(result)
//
//   def decode_7bit_uint32(sysex_bytes):
//       raw = decode_7bit(sysex_bytes)
//       return struct.unpack('>I', raw[:4])[0]
//
//   def crc8_compute(data):
//       crc = 0
//       for b in data:
//           crc ^= b
//           for _ in range(8):
//               crc = (crc << 1) ^ 0x07 if (crc & 0x80) else crc << 1
//               crc &= 0xFF
//       return crc
//
//   def save_and_load(wav_bytes):
//       open('/tmp/voice.wav', 'wb').write(wav_bytes)
//       # Use sf2utils to build a soundfont from the WAV and hot-reload:
//       # build_sf2('/tmp/voice.wav', '/tmp/voice.sf2')
//       # fluidsynth.sfload('/tmp/voice.sf2', reset_presets=True)
//
// ══════════════════════════════════════════════════════
// 7-BIT ENCODING SCHEME
// ══════════════════════════════════════════════════════
//
// MIDI SysEx data bytes must have bit 7 = 0 (values 0x00–0x7F).
// Binary files contain arbitrary byte values (0x00–0xFF).
// We encode using the standard MIDI 7-bit packing:
//
//   7 source bytes → 8 SysEx bytes:
//     SysEx[0]   = MSB bits: (src[0]>>7) | (src[1]>>6) | ... | (src[6]>>1)
//                  bit 0 = MSB of src[0], bit 1 = MSB of src[1], ... bit 6 = MSB of src[6]
//     SysEx[1]   = src[0] & 0x7F
//     SysEx[2]   = src[1] & 0x7F
//     ...
//     SysEx[7]   = src[6] & 0x7F
//
//   Overhead: 8/7 = 14.3% expansion
//
//   Partial group (< 7 source bytes): same scheme applied to however
//   many bytes remain; MSB byte still comes first.
//
// ══════════════════════════════════════════════════════
// MESSAGE REFERENCE
// ══════════════════════════════════════════════════════
//
// All messages:  F0 [0x7D] [0x00] [TYPE] [payload...] F7
//                F0 = SysEx start
//                0x7D = non-commercial manufacturer ID
//                0x00 = PCB Touch Synth device class
//                TYPE = message type byte (below)
//                F7 = SysEx end
//
// TYPE  DIR   NAME              PAYLOAD
// 0x01  D→H   SAMPLE_READY      ASCII filename (null-terminated)
// 0x02  H→D   FILE_REQUEST      (empty)
// 0x03  D→H   FILE_HEADER       file_size(5 bytes 7-bit) block_count(2 bytes raw 7-bit)
// 0x04  D→H   DATA_BLOCK        block_seq(2 raw) [7-bit encoded data] crc8(1 raw)
// 0x05  D→H   TRANSFER_DONE     crc32(5 bytes 7-bit encoded)
// 0x07  D↔H   TRANSFER_ABORT    (empty) — either side may abort
//
// DATA_BLOCK payload:
//   bytes 0–1: block sequence number, big-endian 7-bit (each byte 0x00–0x7F)
//              block_num_hi = (seq >> 7) & 0x7F
//              block_num_lo =  seq       & 0x7F
//   bytes 2–N: SYSEX_BLOCK_ENCODED_LEN bytes of 7-bit encoded source data
//   byte  N+1: CRC-8 of the DECODED block data (before encoding), for integrity check
//              polynomial 0x07, init 0x00 (standard CRC-8/SMBUS)
//
// Block size:  SYSEX_SOURCE_BLOCK_BYTES source bytes per block (63 bytes)
//              Encodes to SYSEX_BLOCK_ENCODED_LEN SysEx bytes (72 bytes)
//              Total DATA_BLOCK message length: 79 bytes (comfortably < 128-byte FIFO)
// ======================================================

#pragma once

#include <stdint.h>

// ---- SysEx frame constants ----
#define SYSEX_MANUFACTURER_ID  0x7Du  // Non-commercial, no registration required
#define SYSEX_DEVICE_ID        0x00u  // PCB Touch Synth identifier

// ---- Message types ----
#define SYSEX_MSG_SAMPLE_READY  0x01u  // Device → Host: new recording available
#define SYSEX_MSG_FILE_REQUEST  0x02u  // Host → Device: please send the file
#define SYSEX_MSG_FILE_HEADER   0x03u  // Device → Host: file size + block count
#define SYSEX_MSG_DATA_BLOCK    0x04u  // Device → Host: one encoded data block
#define SYSEX_MSG_TRANSFER_DONE 0x05u  // Device → Host: all blocks sent + CRC32
#define SYSEX_MSG_ABORT         0x07u  // Either direction: abort transfer

// ---- Block sizing ----
// 63 source bytes → 9 groups × (1 MSB + 7 data) = 72 encoded SysEx bytes.
// Total DATA_BLOCK message: F0(1) hdr(3) seq(2) encoded(72) crc8(1) F7(1) = 80 bytes.
// Fits comfortably in the 128-byte TinyUSB MIDI TX FIFO.
#define SYSEX_SOURCE_BLOCK_BYTES    63u   // binary bytes per block
#define SYSEX_BLOCK_ENCODED_LEN     72u   // 7-bit encoded bytes for 63 source bytes
#define SYSEX_GROUPS_PER_BLOCK       9u   // ceil(63 / 7) groups

// ---- Ring buffer between core 1 (producer) and core 0 (consumer) ----
#define SYSEX_RING_SLOTS       12u    // how many queued SysEx messages core 1 can buffer
#define SYSEX_MAX_MSG_LEN      96u    // generous headroom above 80-byte max message

// ---- File path length for transfer state ----
// Must be >= MAX_PATH_LENGTH in voice_definitions.h (currently 64).
// Defined here so sysex_transfer.cpp and midi_manager.cpp do not need
// to pull in the full voice_definitions.h header.
#define SYSEX_FILEPATH_MAX     64u
