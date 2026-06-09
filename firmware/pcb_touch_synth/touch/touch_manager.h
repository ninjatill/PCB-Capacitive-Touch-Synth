#pragma once

// ======================================================
// TOUCH MANAGER
//
// Reads the MTCH2120 capacitive touch controllers on each
// system tick (interrupt-driven via PIN_TOUCH1_IRQ / PIN_TOUCH2_IRQ)
// and dispatches press/release events to the control manager.
//
// Initialization order requirement:
//   1. board_init()                — configures I2C0 and GPIO
//   2. board_power_release_touch() — de-asserts PIN_TOUCH_RESET
//   3. touch_manager_init()        — initializes MTCH2120 driver and validates device IDs
//
// touch_manager_update() is called from system_tasks() when a touch IRQ is pending.
// ======================================================

bool touch_manager_initialized();

bool touch_manager_init();
void touch_manager_update();

// Reads live button state from both MTCH2120 controllers and prints
// the raw bitmasks plus the name of every pad currently active.
void touch_manager_print_status();