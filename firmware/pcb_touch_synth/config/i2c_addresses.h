#pragma once

#include <stdint.h>

// ======================================================
// I2C BUS ASSIGNMENT SUMMARY
//
// I2C0 (GPIO 4=SDA, GPIO 5=SCL) — Touch controllers only
//   MTCH2120 Touch Controller 1 @ 0x20  (A0=GND, A1=GND)
//   MTCH2120 Touch Controller 2 @ 0x21  (A0=3V3, A1=GND)
//
// I2C1 (GPIO 6=SDA, GPIO 7=SCL) — Audio, Power, LEDs, RTC
//   TLV320DAC3100 Audio DAC     @ 0x18  (SDZ pin selects address)
//   MP2724 Charger/Power Path   @ 0x3F
//   PCA9685  LED Driver 1       @ 0x40  (A0=GND, all others GND)
//   PCA9685  LED Driver 2       @ 0x41  (A0=3V3, all others GND)
//   PCF85063A Real-Time Clock   @ 0x51  (fixed address, no config pins)
//
// No address conflicts exist on either bus.
// ======================================================


// ======================================================
// I2C0 DEVICES (GPIO 4=SDA, GPIO 5=SCL)
// ======================================================

// MTCH2120 Capacitive Touch Controllers (confirmed on PCB)
constexpr uint8_t I2C_ADDR_TOUCH_1  = 0x20;   // A0=GND (touch controller at lower address)
constexpr uint8_t I2C_ADDR_TOUCH_2  = 0x21;   // A0=3V3 (touch controller at higher address)


// ======================================================
// I2C1 DEVICES (GPIO 6=SDA, GPIO 7=SCL)
// ======================================================

// TLV320DAC3100 I2S Class-D Audio DAC / Headphone Amp
// Drives 40mm speaker and 3.5mm headphone jack.
constexpr uint8_t I2C_ADDR_AUDIO_DAC = 0x18;

// MP2724 USB-C Sink Controller / Battery Charger
// Manages USB-C CC negotiation, DPDM detection, and Li-ion charging.
constexpr uint8_t I2C_ADDR_MP2724 = 0x3F;

// PCA9685 16-channel PWM LED Drivers
constexpr uint8_t I2C_ADDR_LED_1  = 0x40;    // A0=GND, all other address pins GND
constexpr uint8_t I2C_ADDR_LED_2  = 0x41;    // A0=3V3, all other address pins GND

// PCF85063A Real-Time Clock/Calendar
// Binary I2C address: 1010001 — fixed, no address-select pins.
// VDD supplied from CHG_OUT via MMSD301T1G Schottky + 150Ω + 1F supercap backup.
// Crystal: Kyocera DT3215DB32768H5HPWAA (32.768 kHz, CL=12.5 pF).
// CAP_SEL register bit must be set to 1 after every power-on reset.
constexpr uint8_t I2C_ADDR_RTC = 0x51;