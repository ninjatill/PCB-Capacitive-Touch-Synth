#pragma once

#include <stdint.h>

// ======================================================
// AUDIO SAMPLE RATE
// ======================================================

constexpr float    AUDIO_SAMPLE_RATE_HZ = 44100.0f;

// ======================================================
// I2S BUS CONFIGURATION
//
// The RP2040 is I2S master; the TLV320DAC3100 is I2S slave.
// The ICS-43432 MEMS microphone also runs as I2S slave on
// the same BCLK / WCLK, with a separate data pin (PIN_MIC_I2S_DATA).
//
// Frame format: 16-bit stereo (I2S standard, MSB-first).
// BCLK = AUDIO_SAMPLE_RATE_HZ × AUDIO_I2S_BCLK_PER_FRAME
//      = 44100 × 32 = 1,411,200 Hz
//
// TLV320 clock tree (MCLK pin is unconnected; PLL input = BCLK):
//   PLL_CLKIN = BCLK = 1,411,200 Hz
//   PLL: P=1, R=8, J=8, D=0 → PLL_CLK = 90,316,800 Hz
//   CODEC_CLKIN = PLL_CLK
//   NDAC=4, MDAC=8, DOSR=64
//   DAC_fS = 90,316,800 / (4 × 8 × 64) = 44,100 Hz ✓
//   Processing block: PRB_P25 (Filter A, stereo, beep generator)
// ======================================================

// BCLK cycles per I2S frame: 64 (32 per channel slot).
// This is mandated by the ICS-43432 MEMS microphone (datasheet p.10:
// "There must be 64 SCK cycles in each WS stereo frame").
// The TLV320DAC3100 is therefore configured for 32-bit word length even though
// audio data is only 24 bits wide. The upper 8 bits of each DAC sample are zero.
// BCLK = 44100 × 64 = 2,822,400 Hz — within both chips' accepted SCK ranges.
constexpr uint32_t AUDIO_I2S_BCLK_PER_FRAME = 64;
constexpr uint32_t AUDIO_BCLK_HZ = (uint32_t)(AUDIO_SAMPLE_RATE_HZ * AUDIO_I2S_BCLK_PER_FRAME);

// ======================================================
// VOICE / POLYPHONY LIMITS
// ======================================================

constexpr uint8_t MAX_POLYPHONY = 8;

constexpr uint8_t MAX_VOICES = 16;
constexpr uint8_t MAX_ACTIVE_NOTES = 8;
constexpr uint8_t MAX_SAMPLES_PER_VOICE = 20;

// ======================================================
// DEFAULT ADSR ENVELOPE
// ======================================================

constexpr uint16_t DEFAULT_ATTACK_MS  = 5;
constexpr uint16_t DEFAULT_DECAY_MS   = 100;
constexpr float    DEFAULT_SUSTAIN    = 0.80f;
constexpr uint16_t DEFAULT_RELEASE_MS = 200;