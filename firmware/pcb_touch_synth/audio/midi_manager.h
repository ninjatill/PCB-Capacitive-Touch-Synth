#pragma once

// ======================================================
// MIDI MANAGER — USB MIDI CONTROLLER OUTPUT
//
// When MIDI mode is enabled the touch synth presents as a USB MIDI
// controller device. Key presses are forwarded to the host PC (Raspberry
// Pi 400) as MIDI Note On / Note Off messages instead of being synthesised
// locally. The host generates audio from those notes and sends it through
// the flyback drivers to plasma speakers.
//
// EXHIBIT CONTEXT
//   Multiple touch synth keyboards connect to a central RP400.
//   Each keyboard is assigned to a MIDI channel (1 = left speaker,
//   2 = right speaker) to produce a stereo effect across the plasma
//   speaker array.  The L/R assignment is toggled by holding the MODE
//   button for MIDI_LR_TOGGLE_HOLD_MS (3 seconds) and is shown on the
//   STATUS_LED_MIDI_LR indicator.
//
// MIDI MODE ACTIVATION
//   MIDI mode is opt-in.  It is NOT automatically entered on USB connection
//   (the USB port doubles as a charging source).
//     Enable:   debug console command  "midi enable"
//     Disable:  debug console command  "midi disable"
//   When enabled: notes → USB MIDI out (local synth silent).
//   When disabled: notes → local synth engine (normal operation).
//
// USB WIRING (Phase 2 — TinyUSB integration required)
//   The RP2040 USB peripheral must be configured as a MIDI device using
//   TinyUSB.  A custom tusb_config.h and USB descriptor file are needed.
//   The USB MUX (PIN_USB_MUX_SEL) routes USB data to the RP2040 when
//   MIDI mode is active.
//   Until TinyUSB is wired, usb_midi_send_packet() is a printf stub and
//   all MIDI state logic is fully functional.
//
// VOICE SAMPLE NOTIFICATION (Phase 3 — file transfer)
//   When a new recording is made in MIDI mode, the device sends a SysEx
//   message to the host indicating the file is ready for download.
//   File transfer itself is a separate mechanism (USB Mass Storage or
//   SysEx bulk transfer).
//
// MIDI CHANNEL ASSIGNMENT
//   MIDI_CHANNEL_LEFT  = 1  (left speaker / plasma coil)
//   MIDI_CHANNEL_RIGHT = 2  (right speaker / plasma coil)
// ======================================================

#include <stdint.h>
#include <stdbool.h>

static constexpr uint8_t MIDI_CHANNEL_LEFT  = 1;
static constexpr uint8_t MIDI_CHANNEL_RIGHT = 2;

// Initialise MIDI manager state. Called from system_init().
// Does not activate MIDI mode — call midi_manager_enable() to do that.
bool midi_manager_init();

// Enable MIDI mode: notes will be forwarded as USB MIDI messages and local
// synthesis is silenced.  Lights STATUS_LED_MODE.
void midi_manager_enable();

// Attempt to enter MIDI mode with USB enumeration and user feedback.
//
//   1. DotStar → MIDI_CONNECTING (fast cyan breathe)
//      MODE LED → rapid blink — "trying to connect"
//   2. Calls board_usb_try_enumerate_midi(2000) — switches USB MUX to RP2040
//      and waits up to 2 s for a host to enumerate the device.
//   3a. Success:
//        Calls midi_manager_enable() (MODE LED solid ON, DotStar → MIDI_ACTIVE).
//        Returns true.
//   3b. Failure:
//        MODE LED → 3 quick flashes then off.
//        DotStar → brief FAULT (red) then restores previous status.
//        Returns false.
//
// This is the function called by both the debug console "midi enable" command
// and the MODE button 10-second long press.
bool midi_manager_try_enable();

// Disable MIDI mode: return to local synthesis.  Extinguishes STATUS_LED_MODE.
void midi_manager_disable();

// True when MIDI mode is active.
bool midi_manager_is_enabled();

// Toggle the L/R channel assignment (called by MODE button 3s hold).
// Left  = MIDI channel 1, Right = MIDI channel 2.
// Updates STATUS_LED_MIDI_LR to reflect the new assignment.
void midi_manager_toggle_lr_channel();

// Return the currently active MIDI channel (MIDI_CHANNEL_LEFT or _RIGHT).
uint8_t midi_manager_get_channel();

// True when the device is assigned to the right channel.
bool midi_manager_is_right_channel();

// Send a Note On message on the active channel.
// midi_note: standard MIDI note number 0–127.
// velocity:  0–127 (127 = full velocity).
void midi_manager_note_on(uint8_t midi_note, uint8_t velocity);

// Send a Note Off message on the active channel.
void midi_manager_note_off(uint8_t midi_note);

// Send a SysEx notification to the host that a new voice recording is
// available for download.  Format:
//   F0 7D 00 01 [filename ASCII bytes] F7
//   0x7D = non-commercial/experimental manufacturer ID
//   0x00 = PCB Touch Synth device identifier
//   0x01 = message type: "sample ready"
// The host RP400 monitors for this message and fetches the file.
void midi_manager_notify_sample_ready(const char* filename);

// Poll the USB MIDI OUT endpoint for incoming packets from the host.
// Call every main-loop iteration (alongside tud_task()) for responsive LEDs.
//
// LIGHT SHOW BEHAVIOUR
//   Filters by the active MIDI channel (1 = left, 2 = right) and maps
//   received note numbers to keyboard key indices based on the current
//   octave offset.  Keys in range [0, 19] are lit on Note On and cleared
//   on Note Off.  Notes outside the visible range are silently ignored.
//
//   Mapping: key_index = midi_note - (MIDI_NOTE_F3 + octave_offset * 12)
//   where MIDI_NOTE_F3 = 53  (F3 = the lowest key on the 20-key keyboard)
//
//   Voice selection is irrelevant for receive — all note numbers on the
//   active channel are processed regardless of the selected voice bank.
//
// Does nothing when MIDI mode is disabled.
void midi_manager_receive_task();

// Print current MIDI state to the debug console.
void midi_manager_print_status();
