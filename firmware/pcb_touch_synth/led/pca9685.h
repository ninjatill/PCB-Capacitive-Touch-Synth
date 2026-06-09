#pragma once

// ======================================================
// PCA9685 16-CHANNEL PWM LED DRIVER
//
// The PCA9685 provides 16 independent 12-bit PWM outputs.
// This project uses two chips on I2C1 (PIN_I2C1_SDA / PIN_I2C1_SCL):
//   LED Driver 1 @ I2C_ADDR_LED_1: octave LEDs + note LEDs (lower range)
//   LED Driver 2 @ I2C_ADDR_LED_2: note LEDs (upper range) + voice/status LEDs
//
// The LED hardware enable line (PIN_LED_EN) must be driven high before LEDs
// will illuminate, even after the PCA9685 is initialized over I2C.
//
// Note: pca9685_i2c is a module-level global; both chips must be on the same bus.
// PWM frequency is set by LED_PWM_FREQUENCY_HZ in firmware_config.h.
// Channel brightness is a 12-bit value (0=off, 4095=full on).
// ======================================================

#include <stdint.h>
#include "hardware/i2c.h"

bool pca9685_init(i2c_inst_t* i2c, uint8_t address);

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