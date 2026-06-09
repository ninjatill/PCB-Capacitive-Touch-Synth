#pragma once

// ======================================================
// MTCH2120 CAPACITIVE TOUCH CONTROLLER DRIVER
//
// The MTCH2120 is a Microchip 16-channel capacitive touch controller.
// This project uses two chips on I2C0 (PIN_I2C0_SDA / PIN_I2C0_SCL):
//   Controller 1 @ I2C_ADDR_TOUCH_1: notes F3-E4       (12 channels used, see touch_map.cpp)
//   Controller 2 @ I2C_ADDR_TOUCH_2: notes F4-C5 + controls (12 channels used, see touch_map.cpp)
//
// Both controllers share a single hardware reset line (PIN_TOUCH_RESET, active low).
// PIN_TOUCH_RESET must be driven high before calling mtch2120_init().
//
// Register access protocol: send 2-byte big-endian register address, then read or write data.
// ======================================================

#include <stdint.h>
#include "hardware/i2c.h"

enum Mtch2120Device {
    TOUCH_CONTROLLER_1,
    TOUCH_CONTROLLER_2
};

bool mtch2120_init(i2c_inst_t* i2c);

bool mtch2120_read_button_status(uint8_t addr, uint16_t* button_mask);
bool mtch2120_calibrate_all(uint8_t addr);

// GPIO12 is the dedicated independent GPIO pin (not a touch button channel).
// Both MTCH2120 devices have this pin routed to the debug header expansion stubs.
// These functions are NOT called during normal init — GPIO12 is left unconfigured
// (factory NVM default: disabled) until the expansion header is actually wired up
// with termination resistors. Call mtch2120_gpio12_init() only when the pin has a
// defined pull to VDD or GND; a floating input reads undefined state.
bool mtch2120_gpio12_init(uint8_t addr);
bool mtch2120_gpio12_read(uint8_t addr, bool* state);

void mtch2120_print_status(uint8_t addr);