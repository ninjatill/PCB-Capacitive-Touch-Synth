#include "status_led.h"
#include "system_tasks.h"
#include "system_interrupts.h"

#include "../touch/touch_manager.h"
#include "../power/power_manager.h"
#include "../audio/audio_manager.h"
#include "../controls/control_manager.h"
#include "../led/led_manager.h"

void system_tasks()
{
    // Periodic/non-interrupt-driven tasks
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

    // Interrupt-driven charger/power handling
    if (system_interrupt_charger_pending()) {
        power_task();
        system_interrupt_clear_charger();
    }
}