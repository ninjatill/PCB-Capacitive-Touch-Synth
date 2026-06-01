#include <stdio.h>

#include "system_init.h"

#include "../audio/audio_manager.h"
#include "../audio/audio_core.h"
#include "../audio/voice_manager.h"
#include "../board/board_init.h"
#include "../controls/control_manager.h"
#include "../led/led_manager.h"
#include "../power/power_manager.h"
#include "../touch/touch_manager.h"
#include "status_led.h"

bool system_init()
{
    status_led_set(SYSTEM_STATUS_INITIALIZING);
    printf("System initialization starting...\n");

    if (!board_init()) {
        status_led_set(SYSTEM_STATUS_FAULT);
        printf("Board initialization failed.\n");
        return false;
    }

    if (!power_startup()) {
        status_led_set(SYSTEM_STATUS_FAULT);
        printf("Power startup failed.\n");
        return false;
    }

    if (!led_manager_init()) {
        status_led_set(SYSTEM_STATUS_FAULT);
        printf("LED manager initialization failed.\n");
        return false;
    }

    if (!audio_core_init()) {
        status_led_set(SYSTEM_STATUS_FAULT);
        printf("Audio core initialization failed.\n");
        return false;
    }

    if (!voice_manager_init()) {
        status_led_set(SYSTEM_STATUS_FAULT);
        printf("Voice manager initialization failed.\n");
        return false;
    }

    if (!audio_manager_init()) {
        status_led_set(SYSTEM_STATUS_FAULT);
        printf("Audio manager initialization failed.\n");
        return false;
    }

    if (!control_manager_init()) {
        status_led_set(SYSTEM_STATUS_FAULT);
        printf("Control manager initialization failed.\n");
        return false;
    }

    if (!touch_manager_init()) {
        status_led_set(SYSTEM_STATUS_FAULT);
        printf("Touch manager initialization failed.\n");
        return false;
    }

    audio_core_start_on_core1();

    status_led_set(SYSTEM_STATUS_OK);
    printf("System initialization complete.\n");

    return true;
}