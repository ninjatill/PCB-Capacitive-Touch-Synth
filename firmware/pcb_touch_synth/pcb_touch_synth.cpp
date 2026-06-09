#include <stdio.h>

#include "pico/stdlib.h"

// ======================================================
// PCB TOUCH SYNTH — MAIN ENTRY POINT
//
// Two-core architecture:
//   Core 0 (this file / system_tasks):
//     Touch input, LED output, USB MIDI, power management,
//     debug console, voice/bank/octave selection, SysEx transfer drain.
//
//   Core 1 (launched by audio_core_start_on_core1):
//     I2S audio render loop (oscillator synthesis, ADSR, DMA fill) when
//     in local synthesis mode.  Idle in MIDI mode; used for SysEx file
//     transfer (SD card read → ring buffer) when a transfer is active.
//
// STARTUP SEQUENCE
//   stdio_init_all()       — UART auto-init on GPIO 12/13 (pico_enable_stdio_uart=1)
//   debug_console_init()   — register command handlers
//   status_led_init()      — Dotstar LEDs via SPI0; must precede the USB wait
//   debug_usb_serial_wait()— timed wait (DEBUG_USB_SERIAL_WAIT_MS) so a UART
//                            terminal can connect before boot logs begin
//   system_init()          — full hardware + subsystem initialisation on core 0
//   audio_core_start_on_core1() — launches core 1 render loop (called inside system_init)
//   system_tasks() loop    — core 0 runs forever servicing all non-audio tasks
// ======================================================

#include "config/firmware_config.h"
#include "debug/debug_console.h"

#include "system/status_led.h"
#include "system/system_init.h"
#include "system/system_tasks.h"

static void debug_usb_serial_wait();

int main()
{
    stdio_init_all();

#if ENABLE_DEBUG_CONSOLE
    debug_console_init();
#endif

    // status_led_init() must run before debug_usb_serial_wait() because
    // the wait loop calls status_led_task() (Dotstar SPI write).
    status_led_init();

    // Timed wait so a UART terminal can connect before boot log output begins.
    // Configured by DEBUG_USB_SERIAL_WAIT_MS in firmware_config.h.
    // Set to 0 to skip for production builds.
    debug_usb_serial_wait();

    printf("\n\n");
    printf("=====================================\n");
    printf(" PCB Piano RP2040 firmware starting  \n");
    printf("=====================================\n");

    if (!system_init()) {
        printf("System initialization failed.\n");

        status_led_set(SYSTEM_STATUS_FAULT);

        while (true) {
            status_led_task();
            sleep_ms(10);
        }
    }

    // Core 1 is running audio_core1_main() from this point.
    // Core 0 runs the task loop forever.
    while (true) {
        system_tasks();
        sleep_ms(1);
    }
}

static void debug_usb_serial_wait()
{
#if ENABLE_DEBUG_BOOT_LOGS
    uint32_t start = to_ms_since_boot(get_absolute_time());

    while ((to_ms_since_boot(get_absolute_time()) - start) < DEBUG_USB_SERIAL_WAIT_MS) {
        status_led_task();
        sleep_ms(10);
    }
#endif
}
