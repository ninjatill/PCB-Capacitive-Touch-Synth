#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "ics43432.h"
#include "audio_constants.h"
#include "tlv320dac3100.h"
#include "../config/board_config.h"

// ======================================================
// STATE
// ======================================================

static bool mic_initialized = false;
static bool mic_recording   = false;


// ======================================================
// INTERNAL HELPERS
// ======================================================

// Called by start_recording() and stop_recording() to manage the DAC.
// Muting the DAC digital output prevents speaker/headphone audio during
// capture without triggering de-pop (the output driver stays powered).
static void set_dac_mute(bool mute)
{
    tlv320dac3100_mute_dac(mute);
}


// ======================================================
// PIO / DMA STUBS
//
// These functions are placeholders for the PIO I2S input implementation
// that belongs in i2s_manager.cpp. They are kept here as a clean seam
// so the public API above compiles and behaves predictably today, and
// the PIO work can be dropped in without touching the public interface.
//
// When implementing:
//   - Use one PIO state machine in input (RX) mode.
//   - The mic's LR pin is tied HIGH (3V3) on the PCB → RIGHT channel.
//     Per ICS-43432 datasheet Figure 13, the MSB is output 1 SCK cycle
//     after the WS RISING edge. The PIO program must:
//       1. Wait for WS to go HIGH (rising edge).
//       2. Wait 1 SCK falling edge (the MSB-delay cycle).
//       3. Shift in 24 bits on subsequent SCK rising edges.
//       4. Ignore the remaining 8 Hi-Z cycles of the right half-frame.
//   - Left-justify the 24 bits in a 32-bit FIFO word so sign extension
//     works with arithmetic right-shift: int32_t s = (int32_t)raw >> 8;
//   - Wire a DMA channel from the PIO RX FIFO into a circular buffer for
//     continuous capture without CPU polling.
// ======================================================

static bool pio_mic_init()
{
    // TODO: claim a PIO state machine, load the I2S RX program,
    //       configure pins (BCLK, WS, DATA) and start the SM.
    return true;   // stub — succeeds silently
}

static void pio_mic_start()
{
    // TODO: enable the PIO state machine and start DMA.
}

static void pio_mic_stop()
{
    // TODO: disable the PIO state machine and halt DMA.
}

// Returns true and writes one sample if the PIO RX FIFO has data.
static bool pio_mic_read_sample(int32_t* sample)
{
    // TODO: check pio_sm_is_rx_fifo_empty(); if data, read and sign-extend.
    // int32_t raw = (int32_t)pio_sm_get(pio, sm) >> 8;
    (void)sample;
    return false;   // stub — no data available yet
}


// ======================================================
// PUBLIC API
// ======================================================

bool ics43432_init()
{
    printf("Initializing ICS-43432 microphone...\n");

    // PIN_MIC_I2S_DATA is the mic's serial data output.
    // SCK and WS are shared with the DAC and already owned by the RP2040
    // I2S master output path. The mic listens to those same signals passively.
    gpio_init(PIN_MIC_I2S_DATA);
    gpio_set_dir(PIN_MIC_I2S_DATA, GPIO_IN);

    if (!pio_mic_init()) {
        printf("ICS-43432: PIO init failed.\n");
        return false;
    }

    mic_initialized = true;
    mic_recording   = false;

    printf("ICS-43432 initialized (PIO capture not yet implemented).\n");
    return true;
}

bool ics43432_start_recording()
{
    if (!mic_initialized) {
        printf("ICS-43432: not initialized.\n");
        return false;
    }

    if (mic_recording) {
        return true;
    }

    printf("ICS-43432: starting recording...\n");

    // Mute the DAC so no speaker or headphone audio leaks into the mic.
    // The output driver stays powered — un-muting is instant with no de-pop.
    set_dac_mute(true);

    // Enable PIO capture. BCLK and WS are already running (driven by the
    // RP2040 I2S output path for the DAC). The mic exits standby as soon
    // as it sees SCK activity and begins outputting samples.
    pio_mic_start();

    // Wait for the mic's internal filter to settle.
    // ICS-43432 datasheet: 262,144 SCK cycles → ~93ms at 2,822,400 Hz BCLK.
    // Reading samples before this window produces invalid data.
    printf("ICS-43432: waiting %lu ms for mic startup...\n",
           (unsigned long)ICS43432_STARTUP_MS);
    sleep_ms(ICS43432_STARTUP_MS);

    mic_recording = true;

    printf("ICS-43432: recording started.\n");
    return true;
}

void ics43432_stop_recording()
{
    if (!mic_recording) {
        return;
    }

    printf("ICS-43432: stopping recording.\n");

    pio_mic_stop();

    mic_recording = false;

    // Restore DAC output. The soft-unmute in the TLV320DAC3100 ramps
    // volume over ~5ms at 44100 Hz, so there is no audible pop.
    set_dac_mute(false);

    printf("ICS-43432: recording stopped.\n");
}

bool ics43432_is_recording()
{
    return mic_recording;
}

bool ics43432_read_sample(int32_t* sample)
{
    if (!mic_recording || sample == nullptr) {
        return false;
    }

    return pio_mic_read_sample(sample);
}

uint32_t ics43432_read_samples(int32_t* buffer, uint32_t count)
{
    if (!mic_recording || buffer == nullptr || count == 0) {
        return 0;
    }

    if (count > ICS43432_MAX_BLOCK) {
        count = ICS43432_MAX_BLOCK;
    }

    uint32_t read = 0;

    while (read < count) {
        if (!pio_mic_read_sample(&buffer[read])) {
            break;
        }
        read++;
    }

    return read;
}
