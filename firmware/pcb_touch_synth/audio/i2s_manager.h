#pragma once

// ======================================================
// I2S MANAGER — PIO / DMA AUDIO OUTPUT
//
// Owns the PIO state machine and DMA channel that stream audio samples
// from SRAM to the I2S bus at 44100 Hz, 32-bit stereo.
//
// ARCHITECTURE
//   One PIO state machine runs i2s_tx.pio, generating BCLK (2.822 MHz),
//   LRCLK (44.1 kHz), and shifting serial DATA to the TLV320DAC3100.
//
//   One DMA channel feeds the PIO TX FIFO.  Double-buffering is manual:
//   while DMA plays one half-buffer, audio_core fills the other.
//   i2s_manager_get_next_write_buffer() performs the buffer swap — it blocks
//   until the current DMA transfer completes, immediately restarts DMA on the
//   just-filled buffer, then returns a pointer to the inactive half for
//   core 1 to fill.
//
// USAGE (core 1 render loop)
//
//   // One-time init — called from audio_core_init() on core 0:
//   i2s_manager_init();
//
//   // Called from core 1 on first NOTE_ON:
//   i2s_manager_start_playback();
//
//   // Render loop (core 1):
//   uint32_t* buf = i2s_manager_get_next_write_buffer();  // blocks ~2.9 ms
//   // fill buf[0..HALF_BUFFER_FRAMES*2-1] with stereo 32-bit samples
//   // repeat forever
//
// BUFFER FORMAT
//   HALF_BUFFER_FRAMES stereo frames per half-buffer.
//   Each frame = 2 × uint32_t: index [f*2+0] = left, [f*2+1] = right.
//   Audio samples from synth_engine are 24-bit signed; expand for PIO:
//     dma_word = (uint32_t)(int32_t)sample_24bit << 8;
//   Upper 24 bits carry audio; lower 8 bits are zero-padded.
//
// CLOCK CONTINUITY
//   The TLV320DAC3100 requires uninterrupted BCLK/WCLK.  Once started, the
//   PIO and DMA run continuously.  i2s_manager_stop() and start_recording()
//   are no-ops after start — audio_core fills the buffer with silence instead.
//
// ICS-43432 RX (Phase 2 — not yet implemented)
//   The mic shares BCLK/LRCLK with the DAC.  A second PIO SM will be added
//   to read the mic data pin synchronously with the TX clock.
// ======================================================

#include <stdint.h>
#include <stdbool.h>

// Stereo frames per DMA half-buffer.  Each frame = 2 × uint32_t (L + R).
// At 44100 Hz: 128 frames ≈ 2.9 ms render budget per iteration.
static constexpr uint32_t HALF_BUFFER_FRAMES = 128u;

// Configure PIO and DMA.  Does NOT start the PIO clock.
// Called once from audio_core_init() on core 0.
bool i2s_manager_init();

// Start the PIO clock and DMA.  Called from core 1 on the first NOTE_ON
// so the DMA channel is owned by the core that will drive it.
// Idempotent — safe to call more than once.
void i2s_manager_start_playback();

// No-ops after the clock starts (clock must run continuously).
// Retained for API compatibility with audio_core mode transitions.
void i2s_manager_start_recording();
void i2s_manager_stop();

// Double-buffer swap — call once per render iteration from core 1.
// Blocks until the current DMA transfer finishes (~2.9 ms), then restarts
// DMA on the previously filled buffer and returns a pointer to the
// now-inactive half for core 1 to fill.
// Returns nullptr before i2s_manager_start_playback() is called.
uint32_t* i2s_manager_get_next_write_buffer();

// True after i2s_manager_start_playback() has been called.
bool i2s_manager_is_running();

// Zero-fill the current write buffer (silence during IDLE / RECORDING).
void i2s_manager_write_silence();
