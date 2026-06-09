#include <stdio.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "touch_manager.h"
#include "mtch2120.h"
#include "touch_map.h"
#include "../config/i2c_addresses.h"
#include "../controls/control_manager.h"

// ======================================================
// INTERNAL STATE
// ======================================================

static bool touch_initialized = false;
static uint16_t previous_touch1_state = 0;
static uint16_t previous_touch2_state = 0;

bool touch_manager_initialized()
{
    return touch_initialized;
}


// ======================================================
// HELPERS
// ======================================================

static bool is_pad_active(const TouchPadMap& pad, uint16_t touch1_state, uint16_t touch2_state)
{
    uint16_t mask = (1u << pad.channel);

    if (pad.controller_addr == I2C_ADDR_TOUCH_1) {
        return (touch1_state & mask) != 0;
    }

    if (pad.controller_addr == I2C_ADDR_TOUCH_2) {
        return (touch2_state & mask) != 0;
    }

    return false;
}

static bool was_pad_active(const TouchPadMap& pad)
{
    uint16_t mask = (1u << pad.channel);

    if (pad.controller_addr == I2C_ADDR_TOUCH_1) {
        return (previous_touch1_state & mask) != 0;
    }

    if (pad.controller_addr == I2C_ADDR_TOUCH_2) {
        return (previous_touch2_state & mask) != 0;
    }

    return false;
}

static void handle_pad_pressed(const TouchPadMap& pad)
{
    if (pad.role == TOUCH_ROLE_NOTE) {
        control_manager_note_pressed(pad.note_index, pad.midi_note);
    }
    else if (pad.role == TOUCH_ROLE_CONTROL) {
        control_manager_control_pressed(pad.control);
    }
}

static void handle_pad_released(const TouchPadMap& pad)
{
    if (pad.role == TOUCH_ROLE_NOTE) {
        control_manager_note_released(pad.note_index, pad.midi_note);
    }
    else if (pad.role == TOUCH_ROLE_CONTROL) {
        control_manager_control_released(pad.control);
    }
}


// ======================================================
// PUBLIC API
// ======================================================

void touch_manager_print_status()
{
    uint16_t touch1 = 0;
    uint16_t touch2 = 0;

    bool ok1 = mtch2120_read_button_status(I2C_ADDR_TOUCH_1, &touch1);
    bool ok2 = mtch2120_read_button_status(I2C_ADDR_TOUCH_2, &touch2);

    printf("Touch controllers:\n");
    printf("  TC1 (0x%02X) %s  mask=0x%04X\n",
           I2C_ADDR_TOUCH_1, ok1 ? "OK  " : "FAIL", touch1);
    printf("  TC2 (0x%02X) %s  mask=0x%04X\n",
           I2C_ADDR_TOUCH_2, ok2 ? "OK  " : "FAIL", touch2);

    printf("Active pads:\n");
    bool any = false;

    for (uint8_t i = 0; i < TOUCH_MAP_COUNT; i++) {
        const TouchPadMap& pad = TOUCH_MAP[i];
        uint16_t mask = (1u << pad.channel);
        bool active = false;

        if (pad.controller_addr == I2C_ADDR_TOUCH_1) {
            active = (touch1 & mask) != 0;
        } else if (pad.controller_addr == I2C_ADDR_TOUCH_2) {
            active = (touch2 & mask) != 0;
        }

        if (active) {
            printf("  [%2u] ch%2u  %s\n", i, pad.channel, pad.name);
            any = true;
        }
    }

    if (!any) {
        printf("  (none)\n");
    }
}

bool touch_manager_init()
{
    printf("Initializing touch manager...\n");

    previous_touch1_state = 0;
    previous_touch2_state = 0;

    // MTCH2120 controllers share I2C0 with the PCA9685 LED drivers.
    // They must be released from hardware reset (PIN_TOUCH_RESET high) before this call.
    if (!mtch2120_init(i2c0)) {
        printf("Touch manager: MTCH2120 initialization failed.\n");
        return false;
    }

    printf("Touch map contains %u pads\n", TOUCH_MAP_COUNT);

    for (uint8_t i = 0; i < TOUCH_MAP_COUNT; i++) {
        const TouchPadMap& pad = TOUCH_MAP[i];

        printf("  pad=%u addr=0x%02X ch=%u name=%s\n",
               i,
               pad.controller_addr,
               pad.channel,
               pad.name);
    }

    touch_initialized = true;

    printf("Touch manager initialized.\n");

    return true;
}

void touch_manager_update()
{
    uint16_t current_touch1_state = 0;
    uint16_t current_touch2_state = 0;

    bool ok1 = mtch2120_read_button_status(I2C_ADDR_TOUCH_1, &current_touch1_state);
    bool ok2 = mtch2120_read_button_status(I2C_ADDR_TOUCH_2, &current_touch2_state);

    if (!ok1 || !ok2) {
        printf("Touch manager update failed: unable to read touch controller state\n");
        return;
    }

    for (uint8_t i = 0; i < TOUCH_MAP_COUNT; i++) {
        const TouchPadMap& pad = TOUCH_MAP[i];

        bool was_active = was_pad_active(pad);
        bool is_active = is_pad_active(pad, current_touch1_state, current_touch2_state);

        if (!was_active && is_active) {
            handle_pad_pressed(pad);
        }
        else if (was_active && !is_active) {
            handle_pad_released(pad);
        }
    }

    previous_touch1_state = current_touch1_state;
    previous_touch2_state = current_touch2_state;
}