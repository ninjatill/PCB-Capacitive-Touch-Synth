#include <stdio.h>

#include "power_manager.h"
#include "mp2724.h"
#include "../board/board_power.h"

bool power_startup()
{
    printf("Power subsystem startup...\n");

    // TODO: initialize MP2724 once hardware is available
    // mp2724_init(i2c1);

    // TODO: enable USB-C sink detection
    // mp2724_enable_usb_c_sink_detection();

    printf("Power subsystem startup complete.\n");

    return true;
}

void power_task()
{
    // TODO: handle charger IRQ/status updates
}