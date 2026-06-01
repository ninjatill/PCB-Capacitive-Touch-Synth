#pragma once

#include <stdint.h>
#include "hardware/i2c.h"

enum Mp2724InputLimit {
    MP2724_INPUT_AUTO,
    MP2724_INPUT_500MA,
    MP2724_INPUT_1500MA,
    MP2724_INPUT_3000MA
};

enum Mp2724DetectedCurrent {
    MP2724_DETECTED_UNKNOWN,
    MP2724_DETECTED_500MA,
    MP2724_DETECTED_1500MA,
    MP2724_DETECTED_3000MA
};

bool mp2724_init(i2c_inst_t* i2c);

bool mp2724_read_register(uint8_t reg, uint8_t* value);
bool mp2724_write_register(uint8_t reg, uint8_t value);
bool mp2724_update_bits(uint8_t reg, uint8_t mask, uint8_t value);

bool mp2724_enable_detection();
bool mp2724_disable_watchdog();

bool mp2724_set_input_limit(Mp2724InputLimit limit);
Mp2724DetectedCurrent mp2724_get_detected_input_current();

bool mp2724_is_vin_good();
bool mp2724_is_vin_ready();

void mp2724_print_status();