#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

#include "hardware/watchdog.h"
#include "pico/stdlib.h"

#include "debug_console.h"
#include "../audio/audio_manager.h"
#include "../audio/voice_config_parser.h"
#include "../audio/voice_definitions.h"
#include "../audio/voice_manager.h"
#include "../audio/tlv320dac3100.h"
#include "../audio/ics43432.h"
#include "../audio/synth_engine.h"
#include "../config/firmware_config.h"
#include "../led/led_manager.h"
#include "../power/mp2724.h"
#include "../power/power_manager.h"
#include "../audio/midi_manager.h"
#include "uart_console.h"
#include "../rtc/pcf85063a.h"
#include "../system/status_led.h"
#include "../touch/touch_manager.h"


static constexpr uint8_t COMMAND_BUFFER_SIZE = 96;

static char command_buffer[COMMAND_BUFFER_SIZE];
static uint8_t command_index = 0;

struct DebugCommand {
    const char* command;
    const char* description;
    void (*handler)(const char* args);
};

static void command_help(const char* args);
static void command_status(const char* args);
static void command_power(const char* args);
static void command_mp2724(const char* args);
static void command_led(const char* args);
static void command_audio(const char* args);
static void command_dac(const char* args);
static void command_touch(const char* args);
static void command_voice(const char* args);
static void command_voices(const char* args);
static void command_voicecfg(const char* args);
static void command_notes(const char* args);
static void command_mic(const char* args);
static void command_uart(const char* args);
static void command_midi(const char* args);
static void command_time(const char* args);
static void command_reboot(const char* args);
static void command_mem(const char* args);

static const DebugCommand COMMANDS[] = {
    { "help",   "Show this command list", command_help },
    { "status", "Show firmware/system status", command_status },

    { "mem", "Show compiled firmware size notes / memory budget", command_mem },

    { "power",  "Show power manager state", command_power },
    { "mp2724", "Show MP2724 charger status registers", command_mp2724 },

    { "led",    "LED commands: led wave | led off | led note <0-19> <on/off>", command_led },

    { "audio",  "Show audio manager state (volume, output, headphones, notes)", command_audio },
    { "dac",    "Show TLV320DAC3100 decoded clock/interface/driver config", command_dac },
    { "touch",  "Show live touch pad state from both MTCH2120 controllers", command_touch },

    { "voice",    "Voice commands: voice | voice next | voice set <0-15>", command_voice },
    { "voices",   "List all loaded voice slots", command_voices },
    { "voicecfg", "Show voice config parser statistics", command_voicecfg },
    { "notes",    "Show currently active synth notes", command_notes },
    { "mic",      "Show ICS-43432 microphone state", command_mic },

    { "uart",   "uart — status | enable | disable  (UART debug on GPIO 12/13)", command_uart },
    { "midi",   "midi — status | enable | disable | channel left|right", command_midi },
    { "time",   "time — show RTC | time set YYYY-MM-DD HH:MM:SS [weekday 0-6] (0=Sun)", command_time },

    { "reboot", "Reboot the RP2040", command_reboot },
};

static constexpr uint8_t COMMAND_COUNT =
    sizeof(COMMANDS) / sizeof(COMMANDS[0]);

static void trim_leading_spaces(char** text)
{
    while (**text == ' ') {
        (*text)++;
    }
}

static bool starts_with(const char* text, const char* prefix)
{
    return strncmp(text, prefix, strlen(prefix)) == 0;
}

void debug_console_init()
{
#if ENABLE_DEBUG_CONSOLE
    command_index = 0;
    command_buffer[0] = '\0';

    printf("Debug console enabled. Type 'help'.\n");
#endif
}

void debug_console_task()
{
#if ENABLE_DEBUG_CONSOLE
    int c = getchar_timeout_us(0);

    while (c != PICO_ERROR_TIMEOUT) {
        if (c == '\r' || c == '\n') {
            if (command_index > 0) {
                command_buffer[command_index] = '\0';

                printf("\n> %s\n", command_buffer);

                char* command = command_buffer;
                trim_leading_spaces(&command);

                bool handled = false;

                for (uint8_t i = 0; i < COMMAND_COUNT; i++) {
                    const char* name = COMMANDS[i].command;
                    size_t len = strlen(name);

                    if (strncmp(command, name, len) == 0 &&
                        (command[len] == '\0' || command[len] == ' ')) {

                        char* args = command + len;
                        trim_leading_spaces(&args);

                        COMMANDS[i].handler(args);
                        handled = true;
                        break;
                    }
                }

                if (!handled) {
                    printf("Unknown command. Type 'help'.\n");
                }

                command_index = 0;
                command_buffer[0] = '\0';
            }
        }
        else if (c == '\b' || c == 127) {
            if (command_index > 0) {
                command_index--;
                command_buffer[command_index] = '\0';
                printf("\b \b");
            }
        }
        else if (c >= 32 && c <= 126) {
            if (command_index < COMMAND_BUFFER_SIZE - 1) {
                command_buffer[command_index++] = (char)c;
                command_buffer[command_index] = '\0';
                putchar(c);
            }
        }

        c = getchar_timeout_us(0);
    }
#endif
}

// ======================================================
// COMMAND HANDLERS
// ======================================================

static void command_help(const char* args)
{
    (void)args;

    printf("Available debug commands:\n");

    for (uint8_t i = 0; i < COMMAND_COUNT; i++) {
        printf("  %-8s - %s\n",
               COMMANDS[i].command,
               COMMANDS[i].description);
    }

    printf("\nExamples:\n");
    printf("  power\n");
    printf("  mp2724\n");
    printf("  led wave\n");
    printf("  led note 7 on\n");
    printf("  voice next\n");
    printf("  voice set 3\n");
    printf("  voices\n");
    printf("  voice 12\n");
    printf("  voicecfg\n");
    printf("  mem\n");
}

static void command_status(const char* args)
{
    (void)args;

    printf("System status:\n");

    if (status_led_initialized()) {
        printf("  status_led = %d\n", status_led_get());
    } else {
        printf("  status_led = (not initialized)\n");
    }

    if (power_initialized()) {
        printf("  input_present        = %s\n", power_input_present() ? "true" : "false");
        printf("  high_power_available = %s\n", power_high_power_available() ? "true" : "false");
    } else {
        printf("  power = (not initialized)\n");
    }

    if (audio_manager_initialized()) {
        printf("  volume = %d\n", audio_manager_get_volume());
    } else {
        printf("  volume = (not initialized)\n");
    }

    if (voice_manager_initialized()) {
        printf("  voice = %u\n", voice_manager_get_current());
    } else {
        printf("  voice = (not initialized)\n");
    }
}

static void command_power(const char* args)
{
    (void)args;

    if (!power_initialized()) {
        printf("Power manager: (not initialized)\n");
        return;
    }

    printf("Power manager:\n");
    printf("  input_present        = %s\n", power_input_present() ? "true" : "false");
    printf("  high_power_available = %s\n", power_high_power_available() ? "true" : "false");
}

static void command_mp2724(const char* args)
{
    (void)args;

    if (!power_initialized()) {
        printf("MP2724: (power manager not initialized)\n");
        return;
    }

    mp2724_print_status();
}

static void command_led(const char* args)
{
    if (!led_manager_initialized()) {
        printf("LED manager: (not initialized)\n");
        return;
    }

    if (strcmp(args, "wave") == 0) {
        led_manager_start_startup_wave();
        printf("LED startup wave triggered.\n");
        return;
    }

    if (strcmp(args, "off") == 0) {
        for (uint8_t i = 0; i < 20; i++) {
            led_manager_set_note(i, false);
        }

        led_manager_set_recording(false);
        led_manager_set_mode(false);
        led_manager_set_midi_right(false);

        printf("LEDs off command sent.\n");
        return;
    }

    if (starts_with(args, "note ")) {
        int note = -1;
        char state[8] = { 0 };

        if (sscanf(args, "note %d %7s", &note, state) == 2) {
            if (note < 0 || note > 19) {
                printf("Invalid note index. Use 0-19.\n");
                return;
            }

            if (strcmp(state, "on") == 0) {
                led_manager_set_note((uint8_t)note, true);
                printf("Note LED %d on.\n", note);
                return;
            }

            if (strcmp(state, "off") == 0) {
                led_manager_set_note((uint8_t)note, false);
                printf("Note LED %d off.\n", note);
                return;
            }
        }

        printf("Usage: led note <0-19> <on/off>\n");
        return;
    }

    printf("LED commands:\n");
    printf("  led wave\n");
    printf("  led off\n");
    printf("  led note <0-19> <on/off>\n");
}

static void command_audio(const char* args)
{
    (void)args;

    if (!audio_manager_initialized()) {
        printf("Audio manager: (not initialized)\n");
        return;
    }

    bool hp = audio_manager_headphones_inserted();

    printf("Audio manager:\n");
    printf("  volume   = %d / 100\n", audio_manager_get_volume());
    printf("  output   = %s\n", hp ? "headphones" : "speaker");
    printf("  headphones = %s\n", hp ? "inserted" : "not inserted");
    printf("  notes    = %s\n", synth_engine_notes_active() ? "active" : "silent");
}

static void command_voice(const char* args)
{
    if (!voice_manager_initialized()) {
        printf("Voice manager: (not initialized)\n");
        return;
    }

    if (args[0] == '\0') {
        printf("Current voice: %u\n", voice_manager_get_current());
        return;
    }

    if (args[0] >= '0' && args[0] <= '9') {
        int voice = atoi(args);

        if (voice < 0 || voice >= VOICE_COUNT) {
            printf("Invalid voice. Use 0-%d.\n", VOICE_COUNT - 1);
            return;
        }

        voice_definitions_print_voice((uint8_t)voice);
        return;
    }

    if (strcmp(args, "next") == 0) {
        uint8_t voice = voice_manager_next();
        led_manager_set_voice(voice);
        printf("Voice advanced to %u\n", voice);
        return;
    }

    if (starts_with(args, "set ")) {
        int voice = atoi(args + 4);

        if (voice < 0 || voice >= VOICE_COUNT) {
            printf("Invalid voice. Use 0-%d.\n", VOICE_COUNT - 1);
            return;
        }

        voice_manager_set_current((uint8_t)voice);
        led_manager_set_voice((uint8_t)voice);

        printf("Voice set to %d\n", voice);
        return;
    }

    printf("Voice commands:\n");
    printf("  voice\n");
    printf("  voice next\n");
    printf("  voice set <0-15>\n");
}

static void command_voices(const char* args)
{
    (void)args;

    if (!voice_manager_initialized()) {
        printf("Voice definitions: (voice manager not initialized)\n");
        return;
    }

    voice_definitions_print();
}

static void command_voicecfg(const char* args)
{
    (void)args;

    if (!voice_manager_initialized()) {
        printf("Voice config: (voice manager not initialized)\n");
        return;
    }

    voice_config_print_stats();
}

static void command_dac(const char* args)
{
    (void)args;

    if (!audio_manager_initialized()) {
        printf("DAC: (audio manager not initialized)\n");
        return;
    }

    tlv320dac3100_print_config();
}

static void command_touch(const char* args)
{
    (void)args;

    if (!touch_manager_initialized()) {
        printf("Touch: (not initialized)\n");
        return;
    }

    touch_manager_print_status();
}

static void command_notes(const char* args)
{
    (void)args;

    if (!audio_manager_initialized()) {
        printf("Notes: (audio manager not initialized)\n");
        return;
    }

    synth_engine_print_status();
}

static void command_mic(const char* args)
{
    (void)args;

    printf("Microphone (ICS-43432):\n");
    printf("  recording = %s\n", ics43432_is_recording() ? "YES" : "no");
    printf("  (PIO capture not yet implemented)\n");
}

static void command_uart(const char* args)
{
    // uart         — show status
    // uart enable  — claim UART peripheral, drive GPIO 12/13, mirror printf to UART
    // uart disable — release UART peripheral, return GPIO 12/13 to safe inputs

    if (!args || args[0] == '\0') {
        printf("UART console: %s\n",
               uart_console_is_active() ? "ENABLED (GPIO 12 TX / 13 RX, 115200 baud)"
                                        : "disabled (pins are high-Z inputs)");
        printf("  Connect via: USB-UART adapter, Particle Debugger, Photon/Argon bridge,\n");
        printf("               or RP400 GPIO 14/15 directly (3.3V, no level shifting).\n");
        return;
    }

    if (strcmp(args, "enable") == 0) {
        uart_console_enable();
        // uart_console_enable() prints its own confirmation on both paths.
        return;
    }

    if (strcmp(args, "disable") == 0) {
        uart_console_disable();
        printf("UART disabled. GPIO 12/13 returned to high-Z inputs.\n");
        return;
    }

    printf("Usage: uart | uart enable | uart disable\n");
}

static void command_midi(const char* args)
{
    // midi              — show status
    // midi enable       — enter MIDI controller mode (notes → USB MIDI to host)
    // midi disable      — return to local synthesis (developer override)
    // midi channel left — assign to MIDI channel 1 (left plasma speaker)
    // midi channel right— assign to MIDI channel 2 (right plasma speaker)

    if (!args || args[0] == '\0') {
        midi_manager_print_status();
        printf("  USB MIDI wiring: Phase 2 (TinyUSB tusb_config.h + descriptors needed)\n");
        printf("  MODE button 3s hold toggles L/R channel while MIDI is enabled.\n");
        return;
    }

    if (strcmp(args, "enable") == 0) {
        printf("MIDI: attempting USB enumeration (same as 10s MODE button hold)...\n");
        bool ok = midi_manager_try_enable();
        if (!ok) {
            printf("MIDI enable failed. Check USB connection and try again.\n");
            printf("(Phase 2: TinyUSB integration required for real enumeration.)\n");
        }
        return;
    }

    if (strcmp(args, "disable") == 0) {
        midi_manager_disable();
        printf("MIDI mode disabled. Returning to local synthesis.\n");
        return;
    }

    if (strncmp(args, "channel", 7) == 0) {
        const char* ch = args + 7;
        while (*ch == ' ') ch++;

        if (strcmp(ch, "left") == 0) {
            if (midi_manager_is_right_channel()) midi_manager_toggle_lr_channel();
            printf("MIDI channel: LEFT (ch1) — left plasma speaker.\n");
        } else if (strcmp(ch, "right") == 0) {
            if (!midi_manager_is_right_channel()) midi_manager_toggle_lr_channel();
            printf("MIDI channel: RIGHT (ch2) — right plasma speaker.\n");
        } else {
            printf("Usage: midi channel left | right\n");
        }
        return;
    }

    printf("Usage: midi | midi enable | midi disable | midi channel left|right\n");
}

static void command_time(const char* args)
{
    // "time" — print current RTC time
    // "time set YYYY-MM-DD HH:MM:SS [weekday]"  — set the RTC
    //
    // Weekday is optional (0=Sunday … 6=Saturday per PCF85063A default).
    // If omitted the weekday register is set to 0 (Sunday).

    if (!args || args[0] == '\0') {
        printf("PCF85063A RTC:\n");
        pcf85063a_print_status();
        return;
    }

    if (strncmp(args, "set", 3) != 0) {
        printf("Usage: time | time set YYYY-MM-DD HH:MM:SS [weekday 0-6] (0=Sun)\n");
        return;
    }

    // Parse: "set YYYY-MM-DD HH:MM:SS [wday]"
    int year, month, day, hour, minute, second, weekday = 0;
    int parsed = sscanf(args + 3, " %d-%d-%d %d:%d:%d %d",
                        &year, &month, &day,
                        &hour, &minute, &second,
                        &weekday);

    if (parsed < 6) {
        printf("Error: expected 'time set YYYY-MM-DD HH:MM:SS [weekday]'\n");
        return;
    }

    // Range checks
    if (year < 2000 || year > 2099 ||
        month < 1 || month > 12 ||
        day < 1 || day > 31 ||
        hour < 0 || hour > 23 ||
        minute < 0 || minute > 59 ||
        second < 0 || second > 59 ||
        weekday < 0 || weekday > 6) {
        printf("Error: one or more values out of range.\n");
        printf("  Year  : 2000–2099\n");
        printf("  Month : 1–12\n");
        printf("  Day   : 1–31\n");
        printf("  Hour  : 0–23\n");
        printf("  Min/Sec: 0–59\n");
        printf("  Weekday: 0=Sun 1=Mon 2=Tue 3=Wed 4=Thu 5=Fri 6=Sat\n");
        return;
    }

    RtcTime t;
    t.year    = (uint8_t)(year - 2000);
    t.month   = (uint8_t)month;
    t.day     = (uint8_t)day;
    t.hours   = (uint8_t)hour;
    t.minutes = (uint8_t)minute;
    t.seconds = (uint8_t)second;
    t.weekday = (uint8_t)weekday;
    t.valid   = true;

    if (!pcf85063a_set_time(&t)) {
        printf("Error: RTC write failed — check I2C1 bus.\n");
        return;
    }

    printf("RTC set to %04d-%02d-%02d %02d:%02d:%02d (weekday %d)\n",
           year, month, day, hour, minute, second, weekday);
}

static void command_reboot(const char* args)
{
    (void)args;

    printf("Rebooting...\n");
    sleep_ms(100);
    watchdog_reboot(0, 0, 0);
}

// Linker symbols from pico-sdk/src/rp2_common/pico_standard_link/memmap_default.ld
// These are defined by the RP2040 linker script and resolve to real addresses at link time.
// Declared as char so pointer arithmetic gives byte counts directly.
extern char __flash_binary_start;   // first byte of flash binary image
extern char __flash_binary_end;     // last byte of flash binary image (code + rodata + data image)
extern char __data_start__;         // start of .data section in SRAM
extern char __data_end__;           // end of .data section in SRAM
extern char __bss_start__;          // start of .bss section in SRAM
extern char __bss_end__;            // end of .bss section / start of heap
extern char __HeapLimit;            // upper bound of heap = top of main SRAM (0x20040000)
extern char __StackTop;             // top (highest address) of core 0 stack in SCRATCH_X
extern char __StackLimit;           // lowest valid stack address (guard boundary)

static void command_mem(const char* args)
{
    (void)args;

    // Read the current stack pointer via inline assembly.
    // This must be done in a non-inlined call so the SP reflects the caller’s frame,
    // giving a realistic measure of stack depth at the time ‘mem’ is run.
    uint32_t sp;
    asm volatile ("mov %0, sp" : "=r" (sp));

    // Section sizes derived from linker symbols — exact, no hard-coded values.
    uint32_t flash_used  = (uint32_t)(&__flash_binary_end   - &__flash_binary_start);
    uint32_t data_size   = (uint32_t)(&__data_end__         - &__data_start__);
    uint32_t bss_size    = (uint32_t)(&__bss_end__          - &__bss_start__);
    uint32_t static_ram  = data_size + bss_size;

    uint32_t heap_base   = (uint32_t)&__bss_end__;
    uint32_t heap_limit  = (uint32_t)&__HeapLimit;
    uint32_t heap_total  = heap_limit - heap_base;

    uint32_t stack_top   = (uint32_t)&__StackTop;
    uint32_t stack_limit = (uint32_t)&__StackLimit;
    uint32_t stack_total = stack_top - stack_limit;
    uint32_t stack_used  = stack_top - sp;
    uint32_t stack_free  = sp - stack_limit;

    // Live heap allocator stats from newlib’s mallinfo().
    // arena: total memory managed by malloc (may be 0 if no allocations made yet).
    // uordblks: bytes currently allocated.  fordblks: bytes free in the arena.
    struct mallinfo mi   = mallinfo();
    uint32_t heap_allocd = (uint32_t)mi.uordblks;
    uint32_t heap_free   = (uint32_t)mi.fordblks;

    printf("Memory summary (live readings):\n");

    printf("\nFlash (XIP):\n");
    printf("  binary used = %u bytes (%.1f KB)\n",
           flash_used, flash_used / 1024.0f);

    printf("\nSRAM static sections (data + bss):\n");
    printf("  .data = %u bytes\n", data_size);
    printf("  .bss  = %u bytes\n", bss_size);
    printf("  total = %u bytes (%.1f KB)\n",
           static_ram, static_ram / 1024.0f);

    printf("\nHeap (main SRAM, grows up from end of .bss):\n");
    printf("  base  = 0x%08X\n", heap_base);
    printf("  limit = 0x%08X\n", heap_limit);
    printf("  total = %u bytes (%.1f KB)\n",
           heap_total, heap_total / 1024.0f);
    printf("  allocd= %u bytes\n", heap_allocd);
    printf("  free  = %u bytes (%.1f KB)\n",
           heap_free, heap_free / 1024.0f);

    printf("\nStack (core 0, SCRATCH_X, grows down from top):\n");
    printf("  top   = 0x%08X\n", stack_top);
    printf("  limit = 0x%08X\n", stack_limit);
    printf("  SP    = 0x%08X\n", sp);
    printf("  total = %u bytes (%.1f KB)\n",
           stack_total, stack_total / 1024.0f);
    printf("  used  = %u bytes\n", stack_used);
    printf("  free  = %u bytes\n", stack_free);

    printf("\nRP2040 totals:\n");
    printf("  SRAM  = 264 KB (256 KB main + 4 KB SCRATCH_X + 4 KB SCRATCH_Y)\n");
    printf("  Flash = 16 MB (W25Q128JV external)\n");
}