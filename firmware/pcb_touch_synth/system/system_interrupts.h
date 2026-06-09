#pragma once

// ======================================================
// SYSTEM INTERRUPT MANAGER
//
// Registers GPIO edge-triggered interrupt handlers for:
//   PIN_TOUCH1_IRQ  — MTCH2120 controller 1 touch event (active-low)
//   PIN_TOUCH2_IRQ  — MTCH2120 controller 2 touch event (active-low)
//   PIN_CHARGER_IRQ — MP2724 power event (active-low)
//   PIN_AUDIO_IRQ   — TLV320DAC3100 fault or headphone detect (active-low)
//
// All IRQs set a volatile flag; system_tasks() polls these flags each
// loop and calls the appropriate handler before clearing the flag.
// This keeps all handler logic on core 0 and avoids reentrancy issues.
//
// system_interrupts_init() must be called after all subsystems are
// initialized (end of system_init()) so handlers are not triggered
// before their drivers are ready.
// ======================================================

void system_interrupts_init();

bool system_interrupt_touch1_pending();
bool system_interrupt_touch2_pending();
bool system_interrupt_charger_pending();
bool system_interrupt_audio_pending();

void system_interrupt_clear_touch1();
void system_interrupt_clear_touch2();
void system_interrupt_clear_charger();
void system_interrupt_clear_audio();