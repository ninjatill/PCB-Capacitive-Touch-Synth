#include <stdio.h>

#include "mp2724.h"
#include "../config/i2c_addresses.h"

// ======================================================
// INTERNAL STATE
// ======================================================

static i2c_inst_t* mp2724_i2c = nullptr;

// ======================================================
// REGISTER MAP - USED REGISTERS ONLY
// ======================================================

constexpr uint8_t REG_CHG_CTRL0 = 0x00;
constexpr uint8_t REG_IIN       = 0x01;
constexpr uint8_t REG_CHG_CTRL2 = 0x07;
constexpr uint8_t REG_CHG_CTRL4 = 0x09;
constexpr uint8_t REG_VIN_DET   = 0x0A;
constexpr uint8_t REG_INT_MASK  = 0x10;

constexpr uint8_t REG_STATUS0   = 0x11;
constexpr uint8_t REG_STATUS1   = 0x12;
constexpr uint8_t REG_STATUS2   = 0x13;
constexpr uint8_t REG_STATUS3   = 0x14;
constexpr uint8_t REG_STATUS4   = 0x15;
constexpr uint8_t REG_STATUS5   = 0x16;

// ======================================================
// BIT MASKS
// ======================================================

// REG_IIN 0x01
constexpr uint8_t IIN_MODE_MASK = 0b11100000;
constexpr uint8_t IIN_LIM_MASK  = 0b00011111;

// IIN_MODE values
constexpr uint8_t IIN_MODE_AUTO    = 0b00000000;
constexpr uint8_t IIN_MODE_500MA   = 0b01000000;
constexpr uint8_t IIN_MODE_1500MA  = 0b10000000;
constexpr uint8_t IIN_MODE_3000MA  = 0b11000000;

// REG_CHG_CTRL2 0x07
constexpr uint8_t WATCHDOG_MASK    = 0b00110000;
constexpr uint8_t WATCHDOG_DISABLE = 0b00000000;

// REG_CHG_CTRL4 0x09
constexpr uint8_t CC_CFG_MASK      = 0b01110000;
constexpr uint8_t CC_CFG_SINK_MODE = 0b00000000;

// REG_VIN_DET 0x0A
constexpr uint8_t AUTODPDM_MASK    = 0b00100000;
constexpr uint8_t AUTODPDM_ENABLE  = 0b00100000;

constexpr uint8_t FORCE_CC_MASK    = 0b00000011;
constexpr uint8_t FORCE_CC_AUTO    = 0b00000000;

// REG_INT_MASK 0x10
constexpr uint8_t MASK_CC_INT      = 0b00000100;

// REG_STATUS1 0x12
constexpr uint8_t STATUS1_VIN_GD   = 0b01000000;
constexpr uint8_t STATUS1_VIN_RDY  = 0b00100000;

// ======================================================
// LOW-LEVEL I2C HELPERS
// ======================================================

bool mp2724_read_register(uint8_t reg, uint8_t* value)
{
    if (mp2724_i2c == nullptr) {
        printf("MP2724 read failed: driver not initialized\n");
        return false;
    }

    int result = i2c_write_blocking(
        mp2724_i2c,
        I2C_ADDR_MP2724,
        &reg,
        1,
        true
    );

    if (result != 1) {
        printf("MP2724 read failed during register select, reg=0x%02X\n", reg);
        return false;
    }

    result = i2c_read_blocking(
        mp2724_i2c,
        I2C_ADDR_MP2724,
        value,
        1,
        false
    );

    if (result != 1) {
        printf("MP2724 read failed during data read, reg=0x%02X\n", reg);
        return false;
    }

    return true;
}

bool mp2724_write_register(uint8_t reg, uint8_t value)
{
    if (mp2724_i2c == nullptr) {
        printf("MP2724 write failed: driver not initialized\n");
        return false;
    }

    uint8_t data[2] = { reg, value };

    int result = i2c_write_blocking(
        mp2724_i2c,
        I2C_ADDR_MP2724,
        data,
        2,
        false
    );

    if (result != 2) {
        printf("MP2724 write failed, reg=0x%02X value=0x%02X\n", reg, value);
        return false;
    }

    return true;
}

bool mp2724_update_bits(uint8_t reg, uint8_t mask, uint8_t value)
{
    uint8_t current = 0;

    if (!mp2724_read_register(reg, &current)) {
        return false;
    }

    uint8_t updated = (current & ~mask) | (value & mask);

    if (updated == current) {
        return true;
    }

    return mp2724_write_register(reg, updated);
}

// ======================================================
// DRIVER INIT
// ======================================================

bool mp2724_init(i2c_inst_t* i2c)
{
    printf("Initializing MP2724 driver...\n");

    mp2724_i2c = i2c;

    uint8_t status1 = 0;

    if (!mp2724_read_register(REG_STATUS1, &status1)) {
        printf("MP2724 not detected at address 0x%02X\n", I2C_ADDR_MP2724);
        return false;
    }

    printf("MP2724 detected at address 0x%02X\n", I2C_ADDR_MP2724);
    printf("MP2724 STATUS1 = 0x%02X\n", status1);

    return true;
}

// ======================================================
// CONFIGURATION
// ======================================================

bool mp2724_enable_detection()
{
    printf("Configuring MP2724 USB-C / DPDM detection...\n");

    // Enable CC1/CC2 sink mode.
    if (!mp2724_update_bits(REG_CHG_CTRL4, CC_CFG_MASK, CC_CFG_SINK_MODE)) {
        return false;
    }

    // Enable automatic D+/D- detection.
    if (!mp2724_update_bits(REG_VIN_DET, AUTODPDM_MASK, AUTODPDM_ENABLE)) {
        return false;
    }

    // Allow CC pins to be automatically configured by CC_CFG.
    if (!mp2724_update_bits(REG_VIN_DET, FORCE_CC_MASK, FORCE_CC_AUTO)) {
        return false;
    }

    // Unmask CC interrupt.
    if (!mp2724_update_bits(REG_INT_MASK, MASK_CC_INT, 0x00)) {
        return false;
    }

    printf("MP2724 detection configured.\n");

    return true;
}

bool mp2724_disable_watchdog()
{
    printf("Disabling MP2724 watchdog timer...\n");

    return mp2724_update_bits(
        REG_CHG_CTRL2,
        WATCHDOG_MASK,
        WATCHDOG_DISABLE
    );
}

bool mp2724_set_input_limit(Mp2724InputLimit limit)
{
    uint8_t mode = IIN_MODE_AUTO;

    switch (limit) {
        case MP2724_INPUT_AUTO:
            printf("MP2724 input limit: AUTO\n");
            mode = IIN_MODE_AUTO;
            break;

        case MP2724_INPUT_500MA:
            printf("MP2724 input limit: forced 500mA\n");
            mode = IIN_MODE_500MA;
            break;

        case MP2724_INPUT_1500MA:
            printf("MP2724 input limit: forced 1.5A\n");
            mode = IIN_MODE_1500MA;
            break;

        case MP2724_INPUT_3000MA:
            printf("MP2724 input limit: forced 3.0A\n");
            mode = IIN_MODE_3000MA;
            break;

        default:
            printf("MP2724 input limit: unknown request\n");
            return false;
    }

    return mp2724_update_bits(REG_IIN, IIN_MODE_MASK, mode);
}

// ======================================================
// STATUS HELPERS
// ======================================================

bool mp2724_is_vin_good()
{
    uint8_t status1 = 0;

    if (!mp2724_read_register(REG_STATUS1, &status1)) {
        return false;
    }

    return (status1 & STATUS1_VIN_GD) != 0;
}

bool mp2724_is_vin_ready()
{
    uint8_t status1 = 0;

    if (!mp2724_read_register(REG_STATUS1, &status1)) {
        return false;
    }

    return (status1 & STATUS1_VIN_RDY) != 0;
}

Mp2724DetectedCurrent mp2724_get_detected_input_current()
{
    uint8_t status4 = 0;

    if (!mp2724_read_register(REG_STATUS4, &status4)) {
        return MP2724_DETECTED_UNKNOWN;
    }

    uint8_t cc1 = (status4 >> 6) & 0x03;
    uint8_t cc2 = (status4 >> 4) & 0x03;

    uint8_t strongest = (cc1 > cc2) ? cc1 : cc2;

    switch (strongest) {
        case 0b01:
            return MP2724_DETECTED_500MA;

        case 0b10:
            return MP2724_DETECTED_1500MA;

        case 0b11:
            return MP2724_DETECTED_3000MA;

        default:
            return MP2724_DETECTED_UNKNOWN;
    }
}

void mp2724_print_status()
{
    uint8_t s0 = 0;
    uint8_t s1 = 0;
    uint8_t s2 = 0;
    uint8_t s3 = 0;
    uint8_t s4 = 0;
    uint8_t s5 = 0;

    mp2724_read_register(REG_STATUS0, &s0);
    mp2724_read_register(REG_STATUS1, &s1);
    mp2724_read_register(REG_STATUS2, &s2);
    mp2724_read_register(REG_STATUS3, &s3);
    mp2724_read_register(REG_STATUS4, &s4);
    mp2724_read_register(REG_STATUS5, &s5);

    printf("MP2724 status registers:\n");
    printf("  STATUS0 0x11 = 0x%02X\n", s0);
    printf("  STATUS1 0x12 = 0x%02X\n", s1);
    printf("  STATUS2 0x13 = 0x%02X\n", s2);
    printf("  STATUS3 0x14 = 0x%02X\n", s3);
    printf("  STATUS4 0x15 = 0x%02X\n", s4);
    printf("  STATUS5 0x16 = 0x%02X\n", s5);

    printf("  VIN_GD  = %u\n", (s1 >> 6) & 0x01);
    printf("  VIN_RDY = %u\n", (s1 >> 5) & 0x01);

    printf("  DPDM_STAT = %u\n", (s0 >> 4) & 0x0F);
    printf("  CC1_STAT  = %u\n", (s4 >> 6) & 0x03);
    printf("  CC2_STAT  = %u\n", (s4 >> 4) & 0x03);

    printf("  CHG_STAT    = %u\n", (s2 >> 5) & 0x07);
    printf("  BOOST_FAULT = %u\n", (s2 >> 2) & 0x07);
    printf("  CHG_FAULT   = %u\n", s2 & 0x03);
}