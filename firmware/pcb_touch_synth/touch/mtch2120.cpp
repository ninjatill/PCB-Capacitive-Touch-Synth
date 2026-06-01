#include <stdio.h>

#include "mtch2120.h"
#include "../config/i2c_addresses.h"

static i2c_inst_t* mtch_i2c = nullptr;

// ======================================================
// MTCH2120 MEMORY MAP - USED REGISTERS ONLY
// ======================================================

constexpr uint16_t REG_DEVID      = 0x0000;
constexpr uint16_t REG_VER        = 0x0001;
constexpr uint16_t REG_DEVSTA     = 0x0100;
constexpr uint16_t REG_BTNSTA     = 0x0102;

constexpr uint16_t REG_SENCTRL_BASE   = 0x0E00;
constexpr uint16_t REG_SIGNAL_BASE    = 0x0200;
constexpr uint16_t REG_REFERENCE_BASE = 0x0300;
constexpr uint16_t REG_SENSTATE_BASE  = 0x0400;

constexpr uint16_t REG_DEVCTRL    = 0x1F00;

// DEVCTRL bits
constexpr uint16_t DEVCTRL_CAL_ALL = 0x0001;
constexpr uint16_t DEVCTRL_RESET   = 0x2000;
constexpr uint16_t DEVCTRL_SAVE    = 0x1000;

// Expected device ID
constexpr uint8_t MTCH2120_DEVID = 0x0B;

// ======================================================
// LOW-LEVEL HELPERS
// ======================================================

static bool mtch2120_select_register(uint8_t addr, uint16_t reg)
{
    uint8_t reg_bytes[2];

    reg_bytes[0] = (uint8_t)((reg >> 8) & 0xFF);  // MSB first
    reg_bytes[1] = (uint8_t)(reg & 0xFF);

    int result = i2c_write_blocking(
        mtch_i2c,
        addr,
        reg_bytes,
        2,
        true
    );

    if (result != 2) {
        printf("MTCH2120 register select failed: addr=0x%02X reg=0x%04X\n", addr, reg);
        return false;
    }

    return true;
}

bool mtch2120_read_u8(uint8_t addr, uint16_t reg, uint8_t* value)
{
    if (mtch_i2c == nullptr) {
        printf("MTCH2120 read failed: driver not initialized\n");
        return false;
    }

    if (!mtch2120_select_register(addr, reg)) {
        return false;
    }

    int result = i2c_read_blocking(
        mtch_i2c,
        addr,
        value,
        1,
        false
    );

    if (result != 1) {
        printf("MTCH2120 read_u8 failed: addr=0x%02X reg=0x%04X\n", addr, reg);
        return false;
    }

    return true;
}

bool mtch2120_read_u16(uint8_t addr, uint16_t reg, uint16_t* value)
{
    uint8_t data[2] = {0, 0};

    if (!mtch2120_select_register(addr, reg)) {
        return false;
    }

    int result = i2c_read_blocking(
        mtch_i2c,
        addr,
        data,
        2,
        false
    );

    if (result != 2) {
        printf("MTCH2120 read_u16 failed: addr=0x%02X reg=0x%04X\n", addr, reg);
        return false;
    }

    *value = (uint16_t)data[0] | ((uint16_t)data[1] << 8);

    return true;
}

bool mtch2120_write_u8(uint8_t addr, uint16_t reg, uint8_t value)
{
    if (mtch_i2c == nullptr) {
        printf("MTCH2120 write failed: driver not initialized\n");
        return false;
    }

    uint8_t data[3];

    data[0] = (uint8_t)((reg >> 8) & 0xFF);
    data[1] = (uint8_t)(reg & 0xFF);
    data[2] = value;

    int result = i2c_write_blocking(
        mtch_i2c,
        addr,
        data,
        3,
        false
    );

    if (result != 3) {
        printf("MTCH2120 write_u8 failed: addr=0x%02X reg=0x%04X value=0x%02X\n", addr, reg, value);
        return false;
    }

    return true;
}

bool mtch2120_write_u16(uint8_t addr, uint16_t reg, uint16_t value)
{
    uint8_t data[4];

    data[0] = (uint8_t)((reg >> 8) & 0xFF);
    data[1] = (uint8_t)(reg & 0xFF);
    data[2] = (uint8_t)(value & 0xFF);
    data[3] = (uint8_t)((value >> 8) & 0xFF);

    int result = i2c_write_blocking(
        mtch_i2c,
        addr,
        data,
        4,
        false
    );

    if (result != 4) {
        printf("MTCH2120 write_u16 failed: addr=0x%02X reg=0x%04X value=0x%04X\n", addr, reg, value);
        return false;
    }

    return true;
}

// ======================================================
// DRIVER INIT
// ======================================================

bool mtch2120_init(i2c_inst_t* i2c)
{
    printf("Initializing MTCH2120 touch driver...\n");

    mtch_i2c = i2c;

    uint8_t devid1 = 0;
    uint8_t devid2 = 0;

    bool ok1 = mtch2120_read_u8(I2C_ADDR_TOUCH_1, REG_DEVID, &devid1);
    bool ok2 = mtch2120_read_u8(I2C_ADDR_TOUCH_2, REG_DEVID, &devid2);

    printf("Touch controller 1 addr=0x%02X DEVID=0x%02X %s\n",
           I2C_ADDR_TOUCH_1,
           devid1,
           (ok1 && devid1 == MTCH2120_DEVID) ? "OK" : "FAIL");

    printf("Touch controller 2 addr=0x%02X DEVID=0x%02X %s\n",
           I2C_ADDR_TOUCH_2,
           devid2,
           (ok2 && devid2 == MTCH2120_DEVID) ? "OK" : "FAIL");

    return ok1 && ok2 && devid1 == MTCH2120_DEVID && devid2 == MTCH2120_DEVID;
}

// ======================================================
// STATUS
// ======================================================

bool mtch2120_read_button_status(uint8_t addr, uint16_t* button_mask)
{
    return mtch2120_read_u16(addr, REG_BTNSTA, button_mask);
}

bool mtch2120_calibrate_all(uint8_t addr)
{
    printf("MTCH2120 calibrate all sensors: addr=0x%02X\n", addr);

    return mtch2120_write_u16(addr, REG_DEVCTRL, DEVCTRL_CAL_ALL);
}

void mtch2120_print_status(uint8_t addr)
{
    uint8_t devid = 0;
    uint8_t ver = 0;
    uint16_t devsta = 0;
    uint16_t btnsta = 0;

    mtch2120_read_u8(addr, REG_DEVID, &devid);
    mtch2120_read_u8(addr, REG_VER, &ver);
    mtch2120_read_u16(addr, REG_DEVSTA, &devsta);
    mtch2120_read_u16(addr, REG_BTNSTA, &btnsta);

    printf("MTCH2120 addr=0x%02X\n", addr);
    printf("  DEVID  = 0x%02X\n", devid);
    printf("  VER    = 0x%02X\n", ver);
    printf("  DEVSTA = 0x%04X\n", devsta);
    printf("  BTNSTA = 0x%04X\n", btnsta);
}