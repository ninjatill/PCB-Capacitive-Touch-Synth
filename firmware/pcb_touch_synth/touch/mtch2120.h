#pragma once

#include <stdint.h>
#include "hardware/i2c.h"

enum Mtch2120Device {
    TOUCH_CONTROLLER_1,
    TOUCH_CONTROLLER_2
};

bool mtch2120_init(i2c_inst_t* i2c);

bool mtch2120_read_u8(uint8_t addr, uint16_t reg, uint8_t* value);
bool mtch2120_read_u16(uint8_t addr, uint16_t reg, uint16_t* value);

bool mtch2120_write_u8(uint8_t addr, uint16_t reg, uint8_t value);
bool mtch2120_write_u16(uint8_t addr, uint16_t reg, uint16_t value);

bool mtch2120_read_button_status(uint8_t addr, uint16_t* button_mask);
bool mtch2120_calibrate_all(uint8_t addr);

void mtch2120_print_status(uint8_t addr);