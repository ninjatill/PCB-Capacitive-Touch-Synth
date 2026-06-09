#pragma once

// ======================================================
// ICS-43432 MEMS MICROPHONE DRIVER
//
// The ICS-43432 is a digital I2S bottom-port microphone with a
// 24-bit two's-complement output at -26 dBFS sensitivity and 65 dBA SNR.
//
// I2S interface (slave, RP2040 is master):
//   SCK  → PIN_AUDIO_I2S_BCLK  (shared with TLV320DAC3100)
//   WS   → PIN_AUDIO_I2S_LRCLK (shared with TLV320DAC3100)
//   SD   ← PIN_MIC_I2S_DATA    (mic data output, dedicated pin)
//
//   Frame format: 64 SCK cycles per WS period (32 per channel slot).
//   Data:         24-bit, MSB first, MSB delayed 1 SCK from WS edge.
//                 The remaining 8 cycles per slot are Hi-Z.
//   BCLK:         AUDIO_BCLK_HZ = AUDIO_SAMPLE_RATE_HZ × 64
//
// Recording workflow:
//   1. ics43432_start_recording() — mutes DAC, starts PIO I2S input,
//      waits ICS43432_STARTUP_MS for the mic to produce valid data.
//   2. Call ics43432_read_sample() or ics43432_read_samples() each tick
//      to drain the PIO RX FIFO.
//   3. ics43432_stop_recording() — stops PIO, un-mutes DAC.
//
// PIO implementation notes (for when i2s_manager adds mic support):
//   - LR pin is tied to 3V3 on the PCB → mic drives the RIGHT channel.
//     Data MSB appears 1 SCK cycle after the WS RISING edge (Figure 13,
//     ICS-43432 datasheet). Sync capture to WS rising edge, skip 1 SCK,
//     then shift in 24 bits.
//   - Left-justify 24 bits in the 32-bit FIFO word; sign-extend with
//     arithmetic right-shift: int32_t s = (int32_t)fifo_word >> 8;
//   - Use DMA from PIO RX FIFO to a ring buffer for continuous capture.
// ======================================================

#include <stdint.h>
#include <stdbool.h>

// Time to wait after BCLK/WS become active before mic data is valid.
// ICS-43432 datasheet: 262,144 SCK cycles.
// At BCLK = 2,822,400 Hz: 262,144 / 2,822,400 ≈ 93 ms. Use 100 ms with margin.
constexpr uint32_t ICS43432_STARTUP_MS = 100;

// Maximum supported sample buffer size for ics43432_read_samples().
constexpr uint32_t ICS43432_MAX_BLOCK = 512;

// Initialize GPIO for the mic data pin. Call once from audio_manager_init()
// or system_init() after board_init() has configured the I2S pins.
// Does NOT start the PIO capture — call ics43432_start_recording() for that.
bool ics43432_init();

// Start recording: mutes the DAC output, enables PIO I2S input capture,
// and waits ICS43432_STARTUP_MS for valid data.
// Returns false if already recording or if PIO setup fails.
bool ics43432_start_recording();

// Stop recording: halts PIO capture and un-mutes the DAC output.
void ics43432_stop_recording();

// Returns true while a recording session is active.
bool ics43432_is_recording();

// Non-blocking read of one 24-bit sample, sign-extended to int32_t.
// Returns false if not recording or if the PIO RX FIFO is empty.
bool ics43432_read_sample(int32_t* sample);

// Read up to 'count' samples into 'buffer'. Returns the number of samples
// actually read (may be less than count if the FIFO runs dry).
uint32_t ics43432_read_samples(int32_t* buffer, uint32_t count);
