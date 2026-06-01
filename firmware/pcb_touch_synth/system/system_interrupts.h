#pragma once

void system_interrupts_init();

bool system_interrupt_touch1_pending();
bool system_interrupt_touch2_pending();
bool system_interrupt_charger_pending();
bool system_interrupt_audio_pending();

void system_interrupt_clear_touch1();
void system_interrupt_clear_touch2();
void system_interrupt_clear_charger();
void system_interrupt_clear_audio();