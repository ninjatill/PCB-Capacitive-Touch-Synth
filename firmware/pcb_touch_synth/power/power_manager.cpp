#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "power_manager.h"
#include "mp2724.h"
#include "../board/board_power.h"
#include "../config/i2c_addresses.h"

// ======================================================
// STATE
// ======================================================

static bool      _initialized  = false;
static PowerMode _mode         = POWER_MODE_DETECTING;
static bool      _input_present = false;

// ======================================================
// POWER MODE DERIVATION
//
// Maps the MP2724-detected power source to a system PowerMode.
// The MP2724 sets its own input current limit autonomously after CC/DPDM
// detection — the MCU reads the result and decides which subsystems to enable.
//
// Decision logic (USB current ONLY — not battery state):
//
//   No USB input           → BATTERY  (sustainable full power from battery)
//   Still detecting        → DETECTING (conservative; upgrades on charger IRQ)
//   USB high current       → USB_FULL  (1.5A or 3A or DCP/adapter)
//   USB 500mA confirmed    → USB_500MA
//   USB unknown / 100mA   → USB_100MA
//
// Battery supplementation (the MP2724 PPM automatically draws from battery
// when system load exceeds USB limit) is intentionally IGNORED for mode
// selection.  Relying on battery to reach full power on a limited USB source
// would drain the battery, then degrade the system when it depletes.
// ======================================================

static PowerMode derive_mode(Mp2724PowerSource source)
{
    switch (source) {
        case MP2724_POWER_SOURCE_DISCONNECTED:
            return POWER_MODE_BATTERY;

        case MP2724_POWER_SOURCE_DETECTING:
            return POWER_MODE_DETECTING;

        case MP2724_POWER_SOURCE_USB_HIGH_CURRENT:
        case MP2724_POWER_SOURCE_ADAPTER:
            return POWER_MODE_USB_FULL;

        case MP2724_POWER_SOURCE_USB_500MA:
            return POWER_MODE_USB_500MA;

        case MP2724_POWER_SOURCE_UNKNOWN:
        default:
            return POWER_MODE_USB_100MA;
    }
}

// ======================================================
// PUBLIC API
// ======================================================

bool power_initialized()
{
    return _initialized;
}

PowerMode power_get_mode()
{
    return _mode;
}

bool power_high_power_available()
{
    return _mode == POWER_MODE_USB_FULL || _mode == POWER_MODE_BATTERY;
}

bool power_input_present()
{
    return _input_present;
}

const char* power_mode_name(PowerMode mode)
{
    switch (mode) {
        case POWER_MODE_DETECTING:  return "Detecting";
        case POWER_MODE_USB_100MA:  return "USB ~100mA (limited)";
        case POWER_MODE_USB_500MA:  return "USB 500mA (SDP)";
        case POWER_MODE_USB_FULL:   return "USB full current";
        case POWER_MODE_BATTERY:    return "Battery";
        default:                    return "Unknown";
    }
}

void power_reevaluate_available_power()
{
    Mp2724PowerSource source;

    if (!mp2724_get_power_source(&source)) {
        printf("Power: MP2724 read failed — defaulting to DETECTING mode.\n");
        _mode = POWER_MODE_DETECTING;
        _input_present = false;
        return;
    }

    _input_present = (source != MP2724_POWER_SOURCE_DISCONNECTED);
    _mode = derive_mode(source);

    printf("Power: source=%s → mode=%s\n",
           mp2724_power_source_name(source),
           power_mode_name(_mode));
}

// ======================================================
// APPLY MODE TO HARDWARE
//
// Called after every mode change to update the 5V rail.
// Audio and LED subsystem responses are applied by system_tasks.cpp
// (which includes audio_manager and led_manager) to avoid circular
// dependencies: power_manager should not depend on audio or LED layers.
// ======================================================

static void apply_5v_rail(PowerMode mode)
{
    switch (mode) {
        case POWER_MODE_USB_FULL:
        case POWER_MODE_BATTERY:
            board_power_set_usb_input_limit(
                (mode == POWER_MODE_USB_FULL) ? USB_INPUT_1500MA : USB_INPUT_3000MA);
            break;

        case POWER_MODE_USB_500MA:
            board_power_set_usb_input_limit(USB_INPUT_500MA);
            break;

        case POWER_MODE_DETECTING:
        case POWER_MODE_USB_100MA:
        default:
            board_power_set_usb_input_limit(USB_INPUT_100MA);
            break;
    }
}

// ======================================================
// STARTUP
// ======================================================

bool power_startup()
{
    printf("Power startup...\n");

    board_power_safe_boot();

    if (!mp2724_init(i2c1, I2C_ADDR_MP2724)) {
        printf("Power: MP2724 init failed — assuming limited USB mode.\n");
        _mode = POWER_MODE_USB_100MA;
        _input_present = false;
        apply_5v_rail(_mode);
        _initialized = true;
        return true;  // non-fatal; system continues in limited mode
    }

    // Trigger autonomous CC/DPDM detection in the MP2724.
    // The chip will fire PIN_CHARGER_IRQ when detection completes.
    mp2724_enable_usb_c_sink_detection();
    mp2724_force_dpdm_detection();

    // Brief wait — detection typically completes in 100–300 ms.
    // If still detecting after timeout, we start conservatively and upgrade
    // via the charger IRQ when the MP2724 finishes.
    sleep_ms(200);
    power_reevaluate_available_power();

    // If still detecting, start in conservative mode.
    if (_mode == POWER_MODE_DETECTING) {
        printf("Power: detection still in progress — starting in conservative mode.\n");
        _mode = POWER_MODE_USB_100MA;
    }

    apply_5v_rail(_mode);

    printf("Power startup complete: %s\n", power_mode_name(_mode));

    _initialized = true;
    return true;
}

// ======================================================
// RUNTIME UPDATE (called on PIN_CHARGER_IRQ)
// ======================================================

void power_task()
{
    printf("Power: charger event — re-evaluating.\n");

    PowerMode old_mode = _mode;
    power_reevaluate_available_power();

    if (_mode == old_mode) {
        printf("Power: mode unchanged (%s).\n", power_mode_name(_mode));
        return;
    }

    printf("Power: mode changed %s → %s\n",
           power_mode_name(old_mode),
           power_mode_name(_mode));

    // Apply the 5V rail change immediately.
    // system_tasks.cpp applies audio and LED changes after this returns,
    // keeping power_manager free of audio/LED dependencies.
    apply_5v_rail(_mode);
}
