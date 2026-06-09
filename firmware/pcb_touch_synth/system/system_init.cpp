#include <stdio.h>

#include "system_init.h"
#include "system_interrupts.h"

#include "tusb.h"

#include "../audio/audio_manager.h"
#include "../audio/audio_core.h"
#include "../audio/midi_manager.h"
#include "../audio/sysex_transfer.h"
#include "../audio/sample_loader.h"
#include "../audio/voice_manager.h"
#include "../board/board_init.h"
#include "../board/board_power.h"
#include "../controls/control_manager.h"
#include "../led/led_manager.h"
#include "../power/power_manager.h"
#include "../rtc/pcf85063a.h"
#include "../touch/touch_manager.h"
#include "status_led.h"

// Forward Declarations
static void system_startup_tick();

bool system_init()
{
    status_led_set(SYSTEM_STATUS_INITIALIZING);

#if ENABLE_DEBUG_BOOT_LOGS
    // USB CDC is disabled (TinyUSB MIDI takes the USB peripheral).
    // Enable UART console immediately so boot logs are visible on GPIO 12/13.
    // Connect a USB-UART adapter or Raspberry Pi 400 UART to see output.
    uart_console_enable();
#endif

    printf("System initialization starting...\n");

    // board_init() configures all GPIO pins and peripheral buses (I2C0, I2C1, SPI0).
    // It holds touch and audio subsystems in hardware reset at this point.
    if (!board_init()) {
        status_led_set(SYSTEM_STATUS_FAULT);
        printf("Board initialization failed.\n");
        return false;
    }

    // power_startup() communicates with the MP2724 over I2C1 to detect the USB power
    // source and enable the 5V rail if high-current power is available.
    if (!power_startup()) {
        status_led_set(SYSTEM_STATUS_FAULT);
        printf("Power startup failed.\n");
        return false;
    }

    // Initialise the PCF85063A RTC on I2C1. Non-fatal: a missing or
    // uninitialised RTC just means FatFS timestamps use a fixed epoch and
    // the debug 'time' command will report an unset clock.
    if (!pcf85063a_init()) {
        printf("RTC not available — FatFS timestamps will use fallback epoch.\n");
    }

    // Enable the LED hardware enable pin now that power is established.
    // This must happen before led_manager_init() or the PCA9685 channels will not drive LEDs.
    board_power_enable_leds();

    if (!led_manager_init()) {
        status_led_set(SYSTEM_STATUS_FAULT);
        printf("LED manager initialization failed.\n");
        return false;
    }

    led_manager_start_startup_wave();
    system_startup_tick();

    // Release the TLV320DAC3100 from hardware reset before attempting I2C communication.
    // The datasheet requires at least 1ms after reset de-assertion before register access.
    board_power_release_audio();
    sleep_ms(5);

    if (!audio_core_init()) {
        status_led_set(SYSTEM_STATUS_FAULT);
        printf("Audio core initialization failed.\n");
        return false;
    }

    system_startup_tick();

    if (!voice_manager_init()) {
        status_led_set(SYSTEM_STATUS_FAULT);
        printf("Voice manager initialization failed.\n");
        return false;
    }

    system_startup_tick();

    if (!audio_manager_init()) {
        status_led_set(SYSTEM_STATUS_FAULT);
        printf("Audio manager initialization failed.\n");
        return false;
    }

    system_startup_tick();

    if (!control_manager_init()) {
        status_led_set(SYSTEM_STATUS_FAULT);
        printf("Control manager initialization failed.\n");
        return false;
    }

    // MIDI manager is always initialized; MIDI mode starts disabled.
    // Enable via debug console: 'midi enable'  OR 10-second MODE button hold.
    midi_manager_init();
    sysex_transfer_init();

    // Initialise TinyUSB USB device stack.  The USB MUX (PIN_USB_MUX_SEL)
    // still routes to MP2724 at this point — the MUX switches to RP2040
    // only when midi_manager_try_enable() calls board_power_connect_usb_data().
    // tusb_init() must be called before any tud_* functions are used.
    tusb_init();

    system_startup_tick();

    // Release the MTCH2120 touch controllers from hardware reset.
    // Both controllers share a single reset line (PIN_TOUCH_RESET).
    // The MTCH2120 datasheet specifies ~20 ms startup time after reset release
    // before the device is ready for I2C communication.  25 ms gives margin.
    board_power_release_touch();
    sleep_ms(25);

    if (!touch_manager_init()) {
        status_led_set(SYSTEM_STATUS_FAULT);
        printf("Touch manager initialization failed.\n");
        return false;
    }

    system_startup_tick();

    // -------------------------------------------------------
    // SD CARD + VOICE LOADING
    // Attempt to mount the SD card and load user voices.
    // This is a non-fatal step: failure leaves the built-in
    // default voices (Sine, Square, Saw, User Recording) active.
    // The SD card detect pin (PIN_SD_CARD_DETECT) is read inside
    // sample_loader_init() — no card = immediate graceful return.
    // -------------------------------------------------------
    printf("Loading voices from SD card (if present)...\n");
    sample_loader_init("/voices/voices.cfg");

    system_startup_tick();

    // Register GPIO interrupt handlers for touch IRQs and charger IRQ.
    // This must be called after touch and power subsystems are initialized.
    system_interrupts_init();

    // Apply the initial power mode to audio and LED subsystems now that
    // both are fully initialized.  power_startup() already configured the
    // 5V rail; this step ensures speaker routing and note LED state match
    // the detected USB current capability at boot time.
    {
        PowerMode boot_mode = power_get_mode();
        bool full_power     = (boot_mode == POWER_MODE_USB_FULL ||
                               boot_mode == POWER_MODE_BATTERY);
        bool allow_notes    = full_power || (boot_mode == POWER_MODE_USB_500MA);

        audio_manager_apply_power_mode(full_power);
        led_manager_set_note_leds_enabled(allow_notes);

        printf("System: initial power mode = %s\n", power_mode_name(boot_mode));
    }

    audio_core_start_on_core1();

    status_led_set(SYSTEM_STATUS_OK);
    printf("System initialization complete.\n");

    return true;
}

static void system_startup_tick()
{
    status_led_task();
    led_manager_task();
}