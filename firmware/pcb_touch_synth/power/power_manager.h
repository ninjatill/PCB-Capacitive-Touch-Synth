#pragma once

// ======================================================
// POWER MANAGER
//
// Coordinates USB-C power detection and system power mode decisions.
// Wraps the MP2724 driver (I2C_ADDR_MP2724 on I2C1) and exposes a
// simplified interface to the rest of the system.
//
// ARCHITECTURE
//   The MP2724 negotiates USB input current autonomously via CC and DPDM
//   detection. The MCU's role is observer: it reads detected power capability
//   and enables or disables subsystems accordingly. The MCU never overrides
//   the MP2724's own current limiting.
//
// POWER MODES
//   DETECTING     VIN present but type not yet identified.
//                 Treated conservatively (same as USB_100MA).
//
//   USB_100MA     ~100mA USB source (pre-enumeration, or strict 100mA host,
//                 or battery unavailable / depleted with limited USB).
//                 5V rail OFF → no LEDs, no speaker.  Headphones work.
//
//   USB_500MA     Standard Downstream Port (PC USB port), 500mA confirmed.
//                 5V rail OFF → no speaker, no note LEDs.
//                 Status LEDs only (voice/octave/mode indicators).
//
//   USB_FULL      USB-C 1.5A, 3A, CDP, or DCP/adapter.
//                 5V rail ON → full features (speaker, all LEDs).
//
//   BATTERY       No USB input; device running on battery alone.
//                 5V rail ON → full features (speaker, all LEDs).
//
// STARTUP FLOW
//   power_startup() triggers MP2724 detection, waits briefly for the result,
//   then applies the initial power mode.  If still detecting at timeout, the
//   system starts in DETECTING (conservative) mode and upgrades when the
//   charger IRQ fires to signal detection complete.
//
// RUNTIME UPDATES
//   PIN_CHARGER_IRQ fires on every plug/unplug event.
//   system_tasks() calls power_task() which re-evaluates the power mode.
//   Subsystem changes (5V rail, speaker, LED mode) are applied immediately.
// ======================================================

// Operating power mode based on detected USB source capability.
enum PowerMode {
    POWER_MODE_DETECTING,  // MP2724 detection in progress — conservative limits
    POWER_MODE_USB_100MA,  // ~100mA: 5V OFF, no LEDs, no speaker, headphones only
    POWER_MODE_USB_500MA,  // 500mA SDP: 5V OFF, no speaker, status LEDs only
    POWER_MODE_USB_FULL,   // 1.5A/3A/adapter: 5V ON, speaker, all LEDs
    POWER_MODE_BATTERY     // No USB: 5V ON, speaker, all LEDs
};

bool power_initialized();

// One-time startup: init MP2724, trigger detection, apply initial mode.
bool power_startup();

// Called by system_tasks() when PIN_CHARGER_IRQ fires (plug/unplug event).
// Re-reads MP2724, applies updated power mode to all subsystems.
void power_task();

// Current power mode — updated by power_startup() and power_task().
PowerMode power_get_mode();

// True when the 5V rail is on and full system features are available.
bool power_high_power_available();

// True when any USB input is detected (regardless of current level).
bool power_input_present();

// Re-reads MP2724 status and updates mode state.
// Called internally by power_task(); can also be called directly if needed.
void power_reevaluate_available_power();

// Human-readable mode name for debug output.
const char* power_mode_name(PowerMode mode);
