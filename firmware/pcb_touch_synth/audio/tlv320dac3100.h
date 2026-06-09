#pragma once

// ======================================================
// TLV320DAC3100 I2S CLASS-D AUDIO DAC DRIVER
//
// The TLV320DAC3100 is a stereo audio DAC with integrated
// Class-D speaker amplifier and headphone amplifier.
// In this design it drives:
//   - 40mm on-board speaker (Class-D output)
//   - 3.5mm stereo headphone jack (HP output, with jack detect)
//
// I2C bus:     I2C1 (PIN_I2C1_SDA / PIN_I2C1_SCL), 400kHz
// I2C address: I2C_ADDR_AUDIO_DAC
// Reset pin:   PIN_AUDIO_RESET (active-low; must be driven high before I2C access)
// IRQ pin:     PIN_AUDIO_IRQ (jack detect and fault notification)
//
// Audio data:  I2S slave mode (PIN_AUDIO_I2S_BCLK / PIN_AUDIO_I2S_LRCLK / PIN_AUDIO_I2S_DATA)
//              RP2040 is I2S master via PIO; sample rate = AUDIO_SAMPLE_RATE_HZ.
//
// The TLV320DAC3100 uses a page-based register map. Always use
// tlv320dac3100_read_register() / tlv320dac3100_write_register() which handle
// page switching automatically.
// ======================================================

#include <stdint.h>
#include "hardware/i2c.h"

// ======================================================
// INIT / RESET
// ======================================================

// Full chip init: clock tree, I2S slave mode, DAC, routing, speaker power-up.
// Call after PIN_AUDIO_RESET is de-asserted and at least 1 ms has elapsed.
bool tlv320dac3100_init(i2c_inst_t* i2c, uint8_t address);

// Software reset (self-clearing). Also called by tlv320dac3100_init().
bool tlv320dac3100_soft_reset();

// ======================================================
// OUTPUT SWITCHING
// Speaker is the default output after init.
// Call enable_headphones() when PIN_AUDIO_IRQ fires and
// headphone insertion is confirmed; call enable_speaker()
// on removal. Both functions leave DAC and volume settings intact.
// ======================================================

bool tlv320dac3100_enable_speaker();
bool tlv320dac3100_enable_headphones();

// ======================================================
// VOLUME
// ======================================================

bool tlv320dac3100_set_dac_volume_db(float db);
bool tlv320dac3100_mute_dac(bool mute);

bool tlv320dac3100_set_headphone_volume_db(float db);
bool tlv320dac3100_set_speaker_volume_db(float db);

// ======================================================
// HEADPHONE DETECTION
// enable_headphone_detect() must be called once (done by init).
// headphone_inserted() reads the instantaneous status flag.
// ======================================================

bool tlv320dac3100_enable_headphone_detect();
bool tlv320dac3100_headphone_inserted(bool* inserted);

// ======================================================
// BEEP / KEY-CLICK  (requires PRB_P25 processing block)
// ======================================================

bool tlv320dac3100_play_beep_1khz();

// ======================================================
// DEBUG
// ======================================================

// Reads back and decodes the clock tree, interface, and output driver
// configuration registers. Use this to verify the init sequence succeeded.
void tlv320dac3100_print_config();

// Reads the DAC/HP/SPK power flag registers and interrupt status.
void tlv320dac3100_print_basic_status();