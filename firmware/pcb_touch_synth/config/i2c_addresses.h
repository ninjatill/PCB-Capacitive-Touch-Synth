#pragma once

#include <stdint.h>

// ======================================================
// I2C0 DEVICES
// ======================================================

// Touch Controllers
constexpr uint8_t I2C_ADDR_TOUCH_1  = 0x20;
constexpr uint8_t I2C_ADDR_TOUCH_2  = 0x21;


// ======================================================
// I2C1 DEVICES
// ======================================================

// Audio Amp/DAC
constexpr uint8_t I2C_ADDR_AUDIO_DAC = 0x18;   // TLV320DAC3100

// Power Controller/Battery Charger
constexpr uint8_t I2C_ADDR_MP2724 = 0x3F;

// LED Drivers
constexpr uint8_t I2C_ADDR_LED_1  = 0x40;    // PCA9685, A0=GND
constexpr uint8_t I2C_ADDR_LED_2  = 0x41;    // PCA9685, A0=3V3