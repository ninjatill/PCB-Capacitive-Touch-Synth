#pragma once

#include <stdint.h>
#include "hardware/i2c.h"

bool pca9685_init(i2c_inst_t* i2c, uint8_t address);

bool pca9685_read_register(uint8_t address, uint8_t reg, uint8_t* value);
bool pca9685_write_register(uint8_t address, uint8_t reg, uint8_t value);

bool pca9685_set_pwm_freq(uint8_t address, float freq_hz);

bool pca9685_set_channel_pwm(
    uint8_t address,
    uint8_t channel,
    uint16_t on_count,
    uint16_t off_count
);

bool pca9685_set_channel_brightness(
    uint8_t address,
    uint8_t channel,
    uint16_t brightness
);

bool pca9685_set_channel_on(uint8_t address, uint8_t channel);
bool pca9685_set_channel_off(uint8_t address, uint8_t channel);

bool pca9685_all_off(uint8_t address);
bool pca9685_all_on(uint8_t address);