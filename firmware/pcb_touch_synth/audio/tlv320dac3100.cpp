#include <stdio.h>

#include "pico/stdlib.h"
#include "tlv320dac3100.h"

static i2c_inst_t* tlv_i2c = nullptr;
static uint8_t tlv_addr = 0;
static uint8_t current_page = 0xFF;

// ======================================================
// TLV320DAC3100 REGISTERS
// ======================================================

constexpr uint8_t REG_PAGE_SELECT      = 0x00;
constexpr uint8_t REG_SOFTWARE_RESET   = 0x01;

// Page 0
constexpr uint8_t REG_OT_FLAG          = 0x03;
constexpr uint8_t REG_DAC_FLAG_1       = 0x25;
constexpr uint8_t REG_STICKY_FLAGS_1   = 0x2C;
constexpr uint8_t REG_INT_FLAGS_2      = 0x2E;
constexpr uint8_t REG_DAC_ROUTING      = 0x3F;
constexpr uint8_t REG_DAC_MUTE_CTRL    = 0x40;
constexpr uint8_t REG_DAC_L_VOL        = 0x41;
constexpr uint8_t REG_DAC_R_VOL        = 0x42;
constexpr uint8_t REG_HEADSET_DETECT   = 0x43;

constexpr uint8_t REG_BEEP_L_VOL       = 0x47;
constexpr uint8_t REG_BEEP_R_VOL       = 0x48;
constexpr uint8_t REG_BEEP_LEN_MSB     = 0x49;
constexpr uint8_t REG_BEEP_LEN_MID     = 0x4A;
constexpr uint8_t REG_BEEP_LEN_LSB     = 0x4B;
constexpr uint8_t REG_BEEP_SIN_MSB     = 0x4C;
constexpr uint8_t REG_BEEP_SIN_LSB     = 0x4D;
constexpr uint8_t REG_BEEP_COS_MSB     = 0x4E;
constexpr uint8_t REG_BEEP_COS_LSB     = 0x4F;

// Page 1
constexpr uint8_t REG_HP_DRIVER_CTRL   = 0x1F;
constexpr uint8_t REG_SPK_DRIVER_CTRL  = 0x20;
constexpr uint8_t REG_OUTPUT_ROUTING   = 0x23;
constexpr uint8_t REG_HPL_ANALOG_VOL   = 0x24;
constexpr uint8_t REG_HPR_ANALOG_VOL   = 0x25;
constexpr uint8_t REG_SPK_ANALOG_VOL   = 0x26;


// ======================================================
// LOW-LEVEL I2C
// ======================================================

bool tlv320dac3100_select_page(uint8_t page)
{
    if (tlv_i2c == nullptr) {
        printf("TLV320DAC3100 page select failed: driver not initialized\n");
        return false;
    }

    if (current_page == page) {
        return true;
    }

    uint8_t data[2] = { REG_PAGE_SELECT, page };

    int result = i2c_write_blocking(tlv_i2c, tlv_addr, data, 2, false);

    if (result != 2) {
        printf("TLV320DAC3100 page select failed: page=%u\n", page);
        return false;
    }

    current_page = page;
    return true;
}

bool tlv320dac3100_write_register(uint8_t page, uint8_t reg, uint8_t value)
{
    if (!tlv320dac3100_select_page(page)) {
        return false;
    }

    uint8_t data[2] = { reg, value };

    int result = i2c_write_blocking(tlv_i2c, tlv_addr, data, 2, false);

    if (result != 2) {
        printf("TLV320DAC3100 write failed: page=%u reg=0x%02X value=0x%02X\n",
               page, reg, value);
        return false;
    }

    return true;
}

bool tlv320dac3100_write_registers(uint8_t page, uint8_t start_reg, const uint8_t* data, uint8_t length)
{
    if (length == 0 || data == nullptr) {
        return false;
    }

    if (!tlv320dac3100_select_page(page)) {
        return false;
    }

    uint8_t buffer[32];

    if (length > 31) {
        printf("TLV320DAC3100 write_registers too long: %u\n", length);
        return false;
    }

    buffer[0] = start_reg;

    for (uint8_t i = 0; i < length; i++) {
        buffer[i + 1] = data[i];
    }

    int result = i2c_write_blocking(tlv_i2c, tlv_addr, buffer, length + 1, false);

    if (result != (int)(length + 1)) {
        printf("TLV320DAC3100 burst write failed: page=%u reg=0x%02X len=%u\n",
               page, start_reg, length);
        return false;
    }

    return true;
}

bool tlv320dac3100_read_register(uint8_t page, uint8_t reg, uint8_t* value)
{
    if (value == nullptr) {
        return false;
    }

    if (!tlv320dac3100_select_page(page)) {
        return false;
    }

    int result = i2c_write_blocking(tlv_i2c, tlv_addr, &reg, 1, true);

    if (result != 1) {
        printf("TLV320DAC3100 register select failed: page=%u reg=0x%02X\n", page, reg);
        return false;
    }

    result = i2c_read_blocking(tlv_i2c, tlv_addr, value, 1, false);

    if (result != 1) {
        printf("TLV320DAC3100 read failed: page=%u reg=0x%02X\n", page, reg);
        return false;
    }

    return true;
}


// ======================================================
// INIT / RESET
// ======================================================

bool tlv320dac3100_init(i2c_inst_t* i2c, uint8_t address)
{
    printf("Initializing TLV320DAC3100 at address 0x%02X...\n", address);

    tlv_i2c = i2c;
    tlv_addr = address;
    current_page = 0xFF;

    if (!tlv320dac3100_soft_reset()) {
        printf("TLV320DAC3100 soft reset failed.\n");
        return false;
    }

    printf("TLV320DAC3100 initialized.\n");
    return true;
}

bool tlv320dac3100_soft_reset()
{
    printf("TLV320DAC3100 software reset...\n");

    if (!tlv320dac3100_write_register(0, REG_SOFTWARE_RESET, 0x01)) {
        return false;
    }

    current_page = 0xFF;

    // Datasheet says internal initialization completes within about 1 ms after reset.
    sleep_ms(2);

    return true;
}


// ======================================================
// VOLUME HELPERS
// ======================================================

static uint8_t dac_volume_db_to_reg(float db)
{
    // DAC digital volume: +24 dB to -63.5 dB in 0.5 dB steps.
    if (db > 24.0f) {
        db = 24.0f;
    }

    if (db < -63.5f) {
        db = -63.5f;
    }

    int steps = (int)(db * 2.0f);

    return (uint8_t)steps;
}

static uint8_t analog_atten_db_to_reg(float db)
{
    // Analog output volume uses 0 dB to about -78 dB attenuation.
    // D7 enables routing from analog volume to output driver.
    if (db > 0.0f) {
        db = 0.0f;
    }

    if (db < -78.0f) {
        db = -78.0f;
    }

    int attenuation_steps = (int)((-db) * 2.0f + 0.5f);

    if (attenuation_steps > 127) {
        attenuation_steps = 127;
    }

    return 0x80 | (uint8_t)attenuation_steps;
}

bool tlv320dac3100_set_dac_volume_db(float db)
{
    uint8_t reg = dac_volume_db_to_reg(db);

    printf("TLV320DAC3100 DAC digital volume %.1f dB reg=0x%02X\n", db, reg);

    bool ok_l = tlv320dac3100_write_register(0, REG_DAC_L_VOL, reg);
    bool ok_r = tlv320dac3100_write_register(0, REG_DAC_R_VOL, reg);

    return ok_l && ok_r;
}

bool tlv320dac3100_mute_dac(bool mute)
{
    printf("TLV320DAC3100 DAC mute: %s\n", mute ? "on" : "off");

    // Bit mapping will be refined when we wire full playback setup.
    // 0x0C mutes left/right DAC channels in TI example scripts.
    return tlv320dac3100_write_register(0, REG_DAC_MUTE_CTRL, mute ? 0x0C : 0x00);
}

bool tlv320dac3100_set_headphone_volume_db(float db)
{
    uint8_t reg = analog_atten_db_to_reg(db);

    printf("TLV320DAC3100 headphone analog volume %.1f dB reg=0x%02X\n", db, reg);

    bool ok_l = tlv320dac3100_write_register(1, REG_HPL_ANALOG_VOL, reg);
    bool ok_r = tlv320dac3100_write_register(1, REG_HPR_ANALOG_VOL, reg);

    return ok_l && ok_r;
}

bool tlv320dac3100_set_speaker_volume_db(float db)
{
    uint8_t reg = analog_atten_db_to_reg(db);

    printf("TLV320DAC3100 speaker analog volume %.1f dB reg=0x%02X\n", db, reg);

    return tlv320dac3100_write_register(1, REG_SPK_ANALOG_VOL, reg);
}


// ======================================================
// HEADPHONE DETECTION
// ======================================================

bool tlv320dac3100_enable_headphone_detect()
{
    printf("TLV320DAC3100 enabling headphone detection...\n");

    // Page 0 / Reg 67:
    // D1 enables headphone detection.
    // D4-D2 set headset debounce.
    // This starts conservative; we can tune debounce later.
    return tlv320dac3100_write_register(0, REG_HEADSET_DETECT, 0x0E);
}

bool tlv320dac3100_headphone_inserted(bool* inserted)
{
    if (inserted == nullptr) {
        return false;
    }

    uint8_t flags = 0;

    if (!tlv320dac3100_read_register(0, REG_INT_FLAGS_2, &flags)) {
        return false;
    }

    // Page 0 / Register 46, bit D4 is headset insertion/removal status.
    *inserted = (flags & 0x10) != 0;

    printf("TLV320DAC3100 headphone inserted: %s flags=0x%02X\n",
           *inserted ? "yes" : "no",
           flags);

    return true;
}


// ======================================================
// BEEP / KEY-CLICK
// ======================================================

bool tlv320dac3100_play_beep_1khz()
{
    printf("TLV320DAC3100 configuring 1 kHz beep...\n");

    // Datasheet example for approx 1 kHz, 5 cycles, 48 kHz sample rate:
    // length = 0x0000F0, sine = 0x10B5, cosine = 0x7EE8.
    uint8_t beep_data[] = {
        0x00, // Reg 73 length MSB
        0x00, // Reg 74 length MID
        0xF0, // Reg 75 length LSB
        0x10, // Reg 76 sine MSB
        0xB5, // Reg 77 sine LSB
        0x7E, // Reg 78 cosine MSB
        0xE8  // Reg 79 cosine LSB
    };

    if (!tlv320dac3100_write_registers(0, REG_BEEP_LEN_MSB, beep_data, sizeof(beep_data))) {
        return false;
    }

    // Enable left/right beep volume. Initial value borrowed from TI example flow.
    bool ok_l = tlv320dac3100_write_register(0, REG_BEEP_L_VOL, 0x80);
    bool ok_r = tlv320dac3100_write_register(0, REG_BEEP_R_VOL, 0x80);

    return ok_l && ok_r;
}


// ======================================================
// DEBUG STATUS
// ======================================================

void tlv320dac3100_print_basic_status()
{
    uint8_t ot = 0;
    uint8_t dac_flag = 0;
    uint8_t sticky = 0;
    uint8_t int2 = 0;

    tlv320dac3100_read_register(0, REG_OT_FLAG, &ot);
    tlv320dac3100_read_register(0, REG_DAC_FLAG_1, &dac_flag);
    tlv320dac3100_read_register(0, REG_STICKY_FLAGS_1, &sticky);
    tlv320dac3100_read_register(0, REG_INT_FLAGS_2, &int2);

    printf("TLV320DAC3100 status:\n");
    printf("  OT flag      = 0x%02X\n", ot);
    printf("  DAC flag     = 0x%02X\n", dac_flag);
    printf("  Sticky flags = 0x%02X\n", sticky);
    printf("  INT flags 2  = 0x%02X\n", int2);
}