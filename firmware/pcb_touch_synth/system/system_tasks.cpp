#include "tusb.h"

#include "status_led.h"
#include "system_tasks.h"
#include "system_interrupts.h"

#include "../audio/sysex_transfer.h"
#include "../board/board_usb.h"
#include "../touch/touch_manager.h"
#include "../power/power_manager.h"
#include "../audio/audio_manager.h"
#include "../audio/midi_manager.h"
#include "../controls/control_manager.h"
#include "../led/led_manager.h"
#include "../config/firmware_config.h"

// Apply subsystem changes in response to a power mode.
// Called after power_task() reports a mode change and on initial boot.
// Kept here (not in power_manager) to avoid power → audio/LED dependencies.
static void apply_power_mode_to_subsystems(PowerMode mode)
{
    bool full_power  = (mode == POWER_MODE_USB_FULL || mode == POWER_MODE_BATTERY);
    bool allow_notes = full_power || (mode == POWER_MODE_USB_500MA);

    // Speaker is only permitted on full power (5V rail on).
    audio_manager_apply_power_mode(full_power);

    // Note LEDs: suppressed at 100mA/detecting (5V off = LEDs can't light).
    // Allowed at 500mA (5V on but limited) since status LEDs still work.
    // Full note LEDs at full power.
    led_manager_set_note_leds_enabled(allow_notes);
}

#if ENABLE_DEBUG_CONSOLE
#include "../debug/debug_console.h"
#endif

void system_tasks()
{
    // Drain the SysEx ring buffer before tud_task() so blocks pushed by
    // core 1 this iteration reach the TinyUSB FIFO and go out this USB frame.
    sysex_transfer_drain();

    // TinyUSB USB device stack — must be called every iteration.
    // Handles USB control transfers, IN/OUT endpoint transactions,
    // and fires tud_midi_rx_cb() when MIDI data arrives from the host.
    tud_task();

    // MIDI receive: poll for note-on/off and SysEx (FILE_REQUEST) from host.
    // No-op when MIDI mode is disabled.
    midi_manager_receive_task();

    // Add debug console processing task, if enabled
    #if ENABLE_DEBUG_CONSOLE
        debug_console_task();
    #endif

    // DotStar SPI protection is handled inside dotstar_show() via
    // dotstar_block_spi(). When core 1 has the SD file open, dotstar_show()
    // silently skips the SPI write but preserves led_buffer state so the
    // correct display resumes immediately when the block is lifted.
    status_led_task();
    audio_manager_task();
    control_manager_task();
    led_manager_task();

    // Interrupt-driven touch handling
    if (system_interrupt_touch1_pending() ||
        system_interrupt_touch2_pending()) {

        touch_manager_update();

        system_interrupt_clear_touch1();
        system_interrupt_clear_touch2();
    }

    // Interrupt-driven charger/power handling.
    // Fires on USB plug/unplug detected by the MP2724 (PIN_CHARGER_IRQ).
    //
    // On unplug while MIDI is active:
    //   The USB host is gone — the MIDI connection is dead.  Disable MIDI
    //   mode and re-enable UART so the debug console remains accessible
    //   without any physical reconnection.
    //
    // On plug-in while MIDI was active:
    //   The user must re-enable MIDI manually ('midi enable' or 10s hold)
    //   so that USB enumeration happens deliberately, not automatically.
    if (system_interrupt_charger_pending()) {
        PowerMode old_mode = power_get_mode();
        bool midi_was_active = midi_manager_is_enabled();

        power_task();
        PowerMode new_mode = power_get_mode();

        // Detect USB disconnect while in MIDI mode
        bool usb_disconnected = (old_mode != POWER_MODE_BATTERY)
                             && (new_mode == POWER_MODE_BATTERY
                                 || new_mode == POWER_MODE_DETECTING);

        if (midi_was_active && usb_disconnected) {
            printf("SYSTEM: USB disconnected while in MIDI mode — disabling MIDI.\n");
            midi_manager_disable();

            // Switch USB MUX back to MP2724 for charging/detection
            board_power_disconnect_usb_data();

            // Clear any lit note LEDs that were driven by the host light show
            for (uint8_t i = 0; i < 20; i++) {
                led_manager_set_note(i, false);
            }
        }

        if (new_mode != old_mode) {
            apply_power_mode_to_subsystems(new_mode);
        }

        system_interrupt_clear_charger();
    }
}