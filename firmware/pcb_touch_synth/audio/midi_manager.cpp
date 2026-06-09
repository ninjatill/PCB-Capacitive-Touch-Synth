#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"

#include "midi_manager.h"
#include "sysex_protocol.h"
#include "sysex_transfer.h"
#include "tusb.h"
#include "../board/board_usb.h"
#include "../controls/control_manager.h"
#include "../debug/uart_console.h"
#include "../config/firmware_config.h"
#include "../led/led_manager.h"
#include "../led/led_map.h"
#include "../system/status_led.h"

// MIDI note number of the lowest keyboard key (F3) at zero octave offset.
static constexpr uint8_t MIDI_NOTE_F3 = 53u;
static constexpr uint8_t KEY_COUNT    = 20u;

// ======================================================
// STATE
// ======================================================

static bool    _enabled       = false;
static bool    _right_channel = false;
static bool    _initialized   = false;

// Path of the most recent file notified to the host via SAMPLE_READY.
// Retained so FILE_REQUEST can be matched to the correct file.
static char    _last_notified_filepath[SYSEX_FILEPATH_MAX] = {};

// Exposed to midi_manager_receive_task() (same translation unit)
const char* sysex_last_notified_filepath(void)
{
    return _last_notified_filepath;
}

// ======================================================
// USB MIDI TRANSPORT (Phase 2 — TinyUSB stub)
//
// When TinyUSB MIDI is integrated, replace the body of
// usb_midi_send_packet() with:
//
//   uint8_t cable = 0;
//   tud_midi_stream_write(cable, packet, len);
//
// and add to CMakeLists.txt:
//   target_link_libraries(pcb_touch_synth tinyusb_device tinyusb_board)
//
// Also add tusb_config.h:
//   #define CFG_TUD_MIDI 1
//   #define CFG_TUD_CDC  0   (or 1 for composite CDC+MIDI)
//
// And provide a USB descriptor (usb_descriptors.c) declaring the MIDI
// interface. See pico-examples/usb/device/midi_device for a template.
// ======================================================

// Send a 4-byte USB MIDI Event Packet (CIN + status + data1 + data2).
// Uses tud_midi_packet_write() which expects the full 4-byte packet
// format including the Code Index Number in byte 0.
static void usb_midi_send_note_packet(const uint8_t packet[4])
{
    if (!tud_midi_mounted()) {
        printf("MIDI TX (no host): [%02X %02X %02X %02X]\n",
               packet[0], packet[1], packet[2], packet[3]);
        return;
    }
    if (!tud_midi_packet_write(packet)) {
        printf("MIDI TX: FIFO full, packet dropped.\n");
    }
}

// Send raw MIDI bytes (e.g. SysEx).  tud_midi_stream_write() adds USB framing.
static void usb_midi_send_raw(const uint8_t* data, size_t len)
{
    if (!tud_midi_mounted()) {
        printf("MIDI TX raw (no host): %u bytes\n", (unsigned)len);
        return;
    }
    uint32_t written = tud_midi_stream_write(0, data, (uint32_t)len);
    if (written != len) {
        printf("MIDI TX: stream buffer full (%u/%u sent)\n", written, (unsigned)len);
    }
}

// ======================================================
// MIDI PACKET HELPERS
//
// USB MIDI packets are 4-byte "USB-MIDI Event Packets":
//   Byte 0: Cable Number (upper nibble) | Code Index Number (lower nibble)
//   Byte 1: MIDI status byte
//   Byte 2: MIDI data byte 1
//   Byte 3: MIDI data byte 2
// ======================================================

static void send_note_on(uint8_t channel, uint8_t note, uint8_t velocity)
{
    // MIDI channel is 1-indexed; protocol is 0-indexed
    uint8_t ch = (channel - 1) & 0x0Fu;

    uint8_t pkt[4] = {
        0x09,
        (uint8_t)(0x90u | ch),
        (uint8_t)(note     & 0x7Fu),
        (uint8_t)(velocity & 0x7Fu)
    };
    usb_midi_send_note_packet(pkt);
}

static void send_note_off(uint8_t channel, uint8_t note)
{
    uint8_t ch = (channel - 1) & 0x0Fu;

    uint8_t pkt[4] = {
        0x08,
        (uint8_t)(0x80u | ch),
        (uint8_t)(note & 0x7Fu),
        0x00u
    };
    usb_midi_send_note_packet(pkt);
}

// ======================================================
// PUBLIC API
// ======================================================

bool midi_manager_init()
{
    _enabled       = false;
    _right_channel = false;
    _initialized   = true;

    printf("MIDI manager: initialized (mode=disabled, channel=left/ch1).\n");
    return true;
}

void midi_manager_enable()
{
    if (_enabled) return;

    _enabled = true;

    // Enable the UART debug console before USB transitions away from CDC.
    // This ensures printf() debug output survives the mode switch and is
    // accessible via the physical UART header (GPIO 12/13).
    // The 9 cm UART traces only carry a driven signal from this point onward.
    uart_console_enable();

    printf("MIDI manager: MIDI mode enabled (ch%u — %s).\n",
           midi_manager_get_channel(),
           _right_channel ? "right" : "left");

    // Light the MIDI mode indicator LED.
    led_manager_set_mode(true);
    led_manager_set_midi_right(_right_channel);
}

bool midi_manager_try_enable()
{
    if (_enabled) {
        printf("MIDI: already enabled.\n");
        return true;
    }

    SystemStatus prev_status = status_led_get();

    // ---- Phase 1: "Connecting" feedback ----
    // DotStar: fast cyan breathe — "actively looking for a host"
    // MODE LED: rapid blink — "trying"
    status_led_set(SYSTEM_STATUS_MIDI_CONNECTING);
    led_manager_set_led(LED_ROLE_STATUS, STATUS_LED_MODE,
                        LED_MODE_BLINK, LED_GLOBAL_BRIGHTNESS, 200);

    printf("MIDI: attempting USB enumeration...\n");

    // ---- Phase 2: attempt USB enumeration (2-second timeout) ----
    bool success = board_usb_try_enumerate_midi(2000);

    if (success) {
        // ---- Success feedback ----
        // DotStar: solid purple (MIDI_ACTIVE) — persistent while MIDI is on
        // MODE LED: solid ON (user requirement)
        midi_manager_enable();          // sets MODE LED solid + enables UART
        status_led_set(SYSTEM_STATUS_MIDI_ACTIVE);
        printf("MIDI: mode engaged — ch%u (%s). MODE LED solid ON.\n",
               midi_manager_get_channel(),
               _right_channel ? "right" : "left");
        return true;
    }

    // ---- Failure feedback ----
    // DotStar: brief FAULT (fast red blink, ~1.2 s) then restore prior status
    // MODE LED: 3 quick flashes then off — "it didn't work"
    printf("MIDI: enumeration failed — reverting to local synthesis.\n");

    status_led_set(SYSTEM_STATUS_FAULT);

    for (int i = 0; i < 3; i++) {
        led_manager_set_led(LED_ROLE_STATUS, STATUS_LED_MODE,
                            LED_MODE_ON, LED_GLOBAL_BRIGHTNESS, 0);
        sleep_ms(120);
        led_manager_set_led(LED_ROLE_STATUS, STATUS_LED_MODE,
                            LED_MODE_OFF, LED_GLOBAL_BRIGHTNESS, 0);
        sleep_ms(120);
    }

    // Hold the red FAULT on DotStar briefly so the user registers it
    sleep_ms(600);

    // Restore DotStar to whatever it was before the attempt
    status_led_set(prev_status);

    // Ensure MODE LED is off (not in MIDI mode)
    led_manager_set_mode(false);

    return false;
}

void midi_manager_disable()
{
    if (!_enabled) return;

    _enabled = false;

    printf("MIDI manager: MIDI mode disabled — returning to local synthesis.\n");

    // Return DotStar to normal OK status
    status_led_set(SYSTEM_STATUS_OK);
    led_manager_set_mode(false);
}

bool midi_manager_is_enabled()
{
    return _enabled;
}

void midi_manager_toggle_lr_channel()
{
    _right_channel = !_right_channel;

    printf("MIDI manager: L/R channel → %s (ch%u).\n",
           _right_channel ? "RIGHT" : "LEFT",
           midi_manager_get_channel());

    led_manager_set_midi_right(_right_channel);
}

uint8_t midi_manager_get_channel()
{
    return _right_channel ? MIDI_CHANNEL_RIGHT : MIDI_CHANNEL_LEFT;
}

bool midi_manager_is_right_channel()
{
    return _right_channel;
}

void midi_manager_note_on(uint8_t midi_note, uint8_t velocity)
{
    if (!_enabled) return;
    send_note_on(midi_manager_get_channel(), midi_note, velocity);
}

void midi_manager_note_off(uint8_t midi_note)
{
    if (!_enabled) return;
    send_note_off(midi_manager_get_channel(), midi_note);
}

void midi_manager_notify_sample_ready(const char* filename)
{
    if (!_enabled || !filename) return;

    // Remember so FILE_REQUEST can match this path
    strncpy(_last_notified_filepath, filename, SYSEX_FILEPATH_MAX - 1);
    _last_notified_filepath[SYSEX_FILEPATH_MAX - 1] = '\0';

    // SysEx format: F0 7D 00 01 [ASCII filename bytes] F7
    //   0x7D = non-commercial / educational manufacturer ID
    //   0x00 = PCB Touch Synth device class identifier
    //   0x01 = message type: sample_ready notification
    //
    // The host RP400 listens for this manufacturer SysEx and initiates
    // a file retrieval (USB Mass Storage read or SysEx bulk transfer —
    // Phase 3 implementation detail).

    size_t name_len = strlen(filename);

    // Build the SysEx packet (max filename 60 bytes to stay within sane limits)
    if (name_len > 60) name_len = 60;

    uint8_t sysex[64];
    uint8_t idx = 0;

    sysex[idx++] = 0xF0;  // Start SysEx
    sysex[idx++] = 0x7D;  // Non-commercial manufacturer ID
    sysex[idx++] = 0x00;  // PCB Touch Synth device
    sysex[idx++] = 0x01;  // Message type: sample_ready

    for (size_t i = 0; i < name_len; i++) {
        sysex[idx++] = (uint8_t)filename[i] & 0x7Fu;  // SysEx bytes must be < 0x80
    }

    sysex[idx++] = 0xF7;  // End SysEx

    // SysEx packets use CIN=0x04 (SysEx starts/continues) and 0x05/0x06/0x07
    // for the terminating packet.  Send as a raw stream for simplicity.
    usb_midi_send_raw(sysex, idx);

    printf("MIDI: SysEx sample_ready sent for '%s'.\n", filename);
}

void midi_manager_receive_task()
{
    if (!_enabled) return;
    if (!tud_midi_available()) return;

    // The active MIDI channel determines which messages we respond to.
    // Channel 1 = left speaker assignment, Channel 2 = right.
    uint8_t our_channel = midi_manager_get_channel();  // 1 or 2

    // Base MIDI note for key 0 shifts with the user's octave selection.
    // F3 (MIDI 53) is key 0 at zero offset; one octave = 12 semitones.
    int base_note = (int)MIDI_NOTE_F3 + control_manager_get_octave_offset() * 12;

    // Drain all pending USB MIDI packets (4-byte USB MIDI Event Packets).
    uint8_t pkt[4];
    while (tud_midi_packet_read(pkt)) {
        // USB MIDI packet format:
        //   pkt[0]: cable number (upper nibble) | Code Index Number (lower nibble)
        //   pkt[1]: MIDI status byte
        //   pkt[2]: MIDI data byte 1  (note number for note messages)
        //   pkt[3]: MIDI data byte 2  (velocity for note messages)

        uint8_t status   = pkt[1];
        uint8_t msg_type = status & 0xF0u;
        uint8_t channel  = (uint8_t)((status & 0x0Fu) + 1u);  // 1-indexed

        // Filter: only process messages on our assigned channel
        if (channel != our_channel) continue;

        uint8_t note     = pkt[2] & 0x7Fu;
        uint8_t velocity = pkt[3] & 0x7Fu;

        // ---- SysEx messages from host ----
        if (status == 0xF0u) {
            // Reconstruct the full SysEx from the stream.
            // TinyUSB delivers SysEx via multiple 4-byte packets; for a
            // simple 3-byte payload (header + type + F7) one packet suffices.
            // The receive_task only handles short inbound SysEx (no data blocks
            // come from host except FILE_REQUEST which has no payload).
            if (pkt[1] == 0x7Du &&    // manufacturer ID
                pkt[2] == 0x00u &&    // device ID
                pkt[3] == SYSEX_MSG_FILE_REQUEST) {
                printf("MIDI: received FILE_REQUEST from host.\n");
                // The last recorded filename is stored in the notification
                // that was sent via midi_manager_notify_sample_ready().
                // sysex_transfer_start() knows the filepath from that call.
                const char* path = sysex_last_notified_filepath();
                if (path && path[0] != '\0') {
                    sysex_transfer_start(path);
                } else {
                    printf("MIDI: no pending file — ignoring FILE_REQUEST.\n");
                }
            }
            // Other SysEx types from host (e.g. ABORT) could be handled here
            continue;
        }

        bool note_on  = (msg_type == 0x90u) && (velocity > 0u);
        bool note_off = (msg_type == 0x80u) || ((msg_type == 0x90u) && (velocity == 0u));

        if (!note_on && !note_off) continue;

        // Map received MIDI note to key LED index.
        // key_index = note - base_note.  Valid range: 0–19 (keyboard keys).
        int key_idx = (int)note - base_note;
        if (key_idx < 0 || key_idx >= (int)KEY_COUNT) continue;

        // Light the key LED on Note On; extinguish on Note Off.
        led_manager_set_note((uint8_t)key_idx, note_on);
    }
}

void midi_manager_print_status()
{
    printf("MIDI manager:\n");
    printf("  enabled   = %s\n", _enabled ? "YES" : "no");
    printf("  channel   = %u (%s)\n",
           midi_manager_get_channel(),
           _right_channel ? "right" : "left");
    printf("  USB MIDI  = stub (Phase 2 — TinyUSB not yet wired)\n");
}
