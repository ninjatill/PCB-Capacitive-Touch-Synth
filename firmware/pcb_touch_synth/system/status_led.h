#pragma once

#include <stdint.h>

enum SystemStatus
{
    SYSTEM_STATUS_BOOTING,
    SYSTEM_STATUS_INITIALIZING,
    SYSTEM_STATUS_OK,
    SYSTEM_STATUS_WARNING,
    SYSTEM_STATUS_FAULT,
    SYSTEM_STATUS_USB_CONNECTED,
    SYSTEM_STATUS_CHARGING,
    SYSTEM_STATUS_RECORDING,
    SYSTEM_STATUS_DEV_ACTIVITY,
    SYSTEM_STATUS_STARTING_AUDIO,
    SYSTEM_STATUS_STARTING_TOUCH
};

bool status_led_init();
void status_led_task();

void status_led_set(SystemStatus status);
SystemStatus status_led_get();