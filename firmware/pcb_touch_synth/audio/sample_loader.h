#pragma once

// ======================================================
// SAMPLE LOADER — SD CARD AUDIO FILE MANAGEMENT
//
// Responsible for everything that touches the SD card on behalf of
// the audio system: loading the voice config file, reading audio
// sample files for PLAYBACK and WAVETABLE voices, and writing the
// user's recorded voice back to the SD card.
//
// SD CARD ACCESS RULES
//   The SD card uses SPI0 (PIN_SPI0_*), which is shared with the
//   DotStar LEDs via the 74AHCT2G125 level shifter.
//   Every SPI0 transaction must be bracketed with:
//     dotstar_spi_acquire();   // disable level shifter to isolate DotStar
//     // ... SD card SPI operations ...
//     dotstar_spi_release();   // re-enable level shifter
//   See dotstar.h for details.
//
//   SD card operations are slow and MUST NOT be called from the core 1
//   audio render loop. All file I/O belongs on core 0, typically during
//   system_init() or in response to user actions (voice change, record stop).
//   Load audio data into SRAM buffers first, then signal core 1 when ready.
//
// RESPONSIBILITIES
//
//   Voice config file
//     Read /voices/voices.cfg line-by-line and feed each line to
//     voice_config_parse_line(). Populates the voice_definitions bank
//     with user-configured voices, overwriting built-in defaults.
//
//   Sample preloading (PLAYBACK voices)
//     For each assigned SampleSlot, read the audio file from the SD card
//     into an SRAM buffer. The synth engine on core 1 plays from SRAM at
//     note-on time. Recommended format: 16-bit or 24-bit mono PCM .wav at
//     AUDIO_SAMPLE_RATE_HZ to avoid pitch shift during playback.
//
//   Wavetable loading (WAVETABLE voices)
//     Read a single-cycle waveform file into a small SRAM lookup table.
//     The oscillator engine will index this table instead of computing
//     the waveform mathematically, enabling richer timbres.
//
//   Recording save
//     After ics43432_stop_recording(), the recording ring buffer holds
//     raw 24-bit samples. sample_loader_save_recording() writes them to
//     the SD card as a .wav file so voice slot 15 (VOICE_RECORD_PLAYBACK)
//     can play them back on subsequent presses of the user recording key.
//
// SD card driver and FatFS stubs are active; full I/O is compiled in but
// gated on the FatFS TODO blocks. When the FatFS driver is wired,
// uncomment the blocks in sample_loader.cpp.
// ======================================================

#include <stdint.h>
#include <stdbool.h>

// One-call init: check card detect, mount filesystem, load voice config,
// and preload all sample slots into SRAM. Call from system_init() after
// audio_manager_init(). Returns false if no card is present or mount fails
// — built-in default voices remain active in that case.
bool sample_loader_init(const char* voice_config_path);

// Load and parse the voice config file from the SD card without preloading
// samples. Useful for reloading config after a card swap.
bool sample_loader_load_voice_config(const char* path);

// Preload audio samples for all assigned PLAYBACK voice slots into SRAM.
// Called automatically by sample_loader_init(). Returns the number of
// sample slots successfully loaded.
uint32_t sample_loader_preload_samples();

// Return a pointer to a preloaded sample buffer for a specific voice and
// note index. Sets *out_num_frames and *out_sample_rate on success.
// Returns nullptr if the slot was not preloaded (no card, or unassigned).
const int32_t* sample_loader_get_sample(
    uint8_t   voice_index,
    uint8_t   note_index,
    uint32_t* out_num_frames,
    uint32_t* out_sample_rate
);

// Write the contents of a recording buffer to the SD card as a PCM .wav file.
// Called on core 0 after ics43432_stop_recording().
// 'samples' contains int32_t sign-extended 24-bit values from the mic.
// Returns false if the SD card write fails.
bool sample_loader_save_recording(
    const char*   path,
    const int32_t* samples,
    uint32_t       sample_count
);