#pragma once

// ======================================================
// TinyUSB configuration — PCB Touch Synth USB MIDI Controller
//
// The device presents as a USB MIDI 1.0 device (Audio Class, MIDI
// Streaming subclass).  There is no CDC interface — debug console
// output is available via UART on GPIO 12 (TX) / GPIO 13 (RX)
// using uart_console_enable() or the 'uart enable' debug command.
//
// USB Audio Class with MIDI Streaming:
//   Interface 0: AudioControl  (mandatory header even for MIDI-only)
//   Interface 1: MIDIStreaming  (bidirectional: keyboard → host, host → keyboard)
//     EP 0x81 IN  — MIDI data from keyboard to host (note on/off, etc.)
//     EP 0x01 OUT — MIDI data from host to keyboard (light show, sync, etc.)
// ======================================================

#ifndef CFG_TUSB_MCU
#define CFG_TUSB_MCU          OPT_MCU_RP2040
#endif

#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)
#define CFG_TUSB_OS           OPT_OS_PICO
#define CFG_TUSB_MEM_ALIGN    __attribute__((aligned(4)))

// Endpoint 0 max packet size
#define CFG_TUD_ENDPOINT0_SIZE  64

// Disable all classes except MIDI
#define CFG_TUD_CDC     0
#define CFG_TUD_MSC     0
#define CFG_TUD_HID     0
#define CFG_TUD_VENDOR  0
#define CFG_TUD_AUDIO   0

// One bidirectional MIDI streaming interface
#define CFG_TUD_MIDI    1

// MIDI FIFO sizes (bytes) — 128 is sufficient for typical note traffic
#define CFG_TUD_MIDI_RX_BUFSIZE  128
#define CFG_TUD_MIDI_TX_BUFSIZE  128
