#include <stdio.h>

#include "pico/stdlib.h"
#include "pca9685.h"

static i2c_inst_t* pca9685_i2c = nullptr;

// ======================================================
// PCA9685 REGISTERS
// ======================================================

constexpr uint8_t REG_MODE1       = 0x00;
constexpr uint8_t REG_MODE2       = 0x01;

constexpr uint8_t REG_LED0_ON_L   = 0x06;

constexpr uint8_t REG_ALL_ON_L    = 0xFA;
constexpr uint8_t REG_ALL_ON_H    = 0xFB;
constexpr uint8_t REG_ALL_OFF_L   = 0xFC;
constexpr uint8_t REG_ALL_OFF_H   = 0xFD;

constexpr uint8_t REG_PRE_SCALE   = 0xFE;

// MODE1 bits
constexpr uint8_t MODE1_RESTART   = 0x80;
constexpr uint8_t MODE1_AI        = 0x20;
constexpr uint8_t MODE1_SLEEP     = 0x10;
constexpr uint8_t MODE1_ALLCALL   = 0x01;

// MODE2 bits
constexpr uint8_t MODE2_OUTDRV    = 0x04;

// LED full on/off bit
constexpr uint8_t LED_FULL_BIT    = 0x10;

constexpr float PCA9685_OSC_HZ    = 25000000.0f;


// ======================================================
// LOW-LEVEL I2C
// ======================================================

static bool pca9685_read_register(uint8_t address, uint8_t reg, uint8_t* value)
{
    if (pca9685_i2c == nullptr) {
        printf("PCA9685 read failed: driver not initialized\n");
        return false;
    }

    int result = i2c_write_blocking(
        pca9685_i2c,
        address,
        &reg,
        1,
        true
    );

    if (result != 1) {
        printf("PCA9685 register select failed: addr=0x%02X reg=0x%02X\n", address, reg);
        return false;
    }

    result = i2c_read_blocking(
        pca9685_i2c,
        address,
        value,
        1,
        false
    );

    if (result != 1) {
        printf("PCA9685 read failed: addr=0x%02X reg=0x%02X\n", address, reg);
        return false;
    }

    return true;
}

static bool pca9685_write_register(uint8_t address, uint8_t reg, uint8_t value)
{
    if (pca9685_i2c == nullptr) {
        printf("PCA9685 write failed: driver not initialized\n");
        return false;
    }

    uint8_t data[2] = { reg, value };

    int result = i2c_write_blocking(
        pca9685_i2c,
        address,
        data,
        2,
        false
    );

    if (result != 2) {
        printf("PCA9685 write failed: addr=0x%02X reg=0x%02X value=0x%02X\n",
               address,
               reg,
               value);
        return false;
    }

    return true;
}


// ======================================================
// DRIVER INIT
// ======================================================

bool pca9685_init(i2c_inst_t* i2c, uint8_t address)
{
    printf("Initializing PCA9685 at address 0x%02X...\n", address);

    pca9685_i2c = i2c;

    // Wake oscillator, enable auto-increment, keep all-call enabled.
    if (!pca9685_write_register(address, REG_MODE1, MODE1_AI | MODE1_ALLCALL)) {
        return false;
    }

    sleep_ms(1);

    // Totem-pole outputs by default. This matches the device default and
    // is usually correct when driving MOSFET/transistor gates or logic inputs.
    if (!pca9685_write_register(address, REG_MODE2, MODE2_OUTDRV)) {
        return false;
    }

    if (!pca9685_all_off(address)) {
        return false;
    }

    printf("PCA9685 initialized at address 0x%02X\n", address);

    return true;
}


// ======================================================
// PWM FREQUENCY
// ======================================================

bool pca9685_set_pwm_freq(uint8_t address, float freq_hz)
{
    if (freq_hz < 24.0f) {
        freq_hz = 24.0f;
    }

    if (freq_hz > 1526.0f) {
        freq_hz = 1526.0f;
    }

    float prescale_float = (PCA9685_OSC_HZ / (4096.0f * freq_hz)) - 1.0f;
    uint8_t prescale = (uint8_t)(prescale_float + 0.5f);

    printf("PCA9685 addr=0x%02X setting PWM freq %.1f Hz, prescale=%u\n",
           address,
           freq_hz,
           prescale);

    uint8_t old_mode = 0;

    if (!pca9685_read_register(address, REG_MODE1, &old_mode)) {
        return false;
    }

    uint8_t sleep_mode = (old_mode & ~MODE1_RESTART) | MODE1_SLEEP;

    // PRE_SCALE can only be changed while SLEEP=1.
    if (!pca9685_write_register(address, REG_MODE1, sleep_mode)) {
        return false;
    }

    if (!pca9685_write_register(address, REG_PRE_SCALE, prescale)) {
        return false;
    }

    if (!pca9685_write_register(address, REG_MODE1, old_mode)) {
        return false;
    }

    // Oscillator needs up to 500us after waking.
    sleep_ms(1);

    if (!pca9685_write_register(address, REG_MODE1, old_mode | MODE1_RESTART | MODE1_AI)) {
        return false;
    }

    return true;
}


// ======================================================
// CHANNEL CONTROL
// ======================================================

bool pca9685_set_channel_pwm(
    uint8_t address,
    uint8_t channel,
    uint16_t on_count,
    uint16_t off_count
)
{
    if (channel > 15) {
        printf("PCA9685 invalid channel: %u\n", channel);
        return false;
    }

    on_count &= 0x0FFF;
    off_count &= 0x0FFF;

    uint8_t reg = REG_LED0_ON_L + (4 * channel);

    uint8_t data[5];
    data[0] = reg;
    data[1] = (uint8_t)(on_count & 0xFF);
    data[2] = (uint8_t)((on_count >> 8) & 0x0F);
    data[3] = (uint8_t)(off_count & 0xFF);
    data[4] = (uint8_t)((off_count >> 8) & 0x0F);

    int result = i2c_write_blocking(
        pca9685_i2c,
        address,
        data,
        5,
        false
    );

    if (result != 5) {
        printf("PCA9685 set PWM failed: addr=0x%02X ch=%u\n", address, channel);
        return false;
    }

    return true;
}

bool pca9685_set_channel_brightness(
    uint8_t address,
    uint8_t channel,
    uint16_t brightness
)
{
    if (brightness == 0) {
        return pca9685_set_channel_off(address, channel);
    }

    if (brightness >= 4095) {
        return pca9685_set_channel_on(address, channel);
    }

    return pca9685_set_channel_pwm(address, channel, 0, brightness);
}

bool pca9685_set_channel_on(uint8_t address, uint8_t channel)
{
    if (channel > 15) {
        return false;
    }

    uint8_t reg = REG_LED0_ON_L + (4 * channel);

    uint8_t data[5];
    data[0] = reg;
    data[1] = 0x00;
    data[2] = LED_FULL_BIT;
    data[3] = 0x00;
    data[4] = 0x00;

    int result = i2c_write_blocking(pca9685_i2c, address, data, 5, false);

    return result == 5;
}

bool pca9685_set_channel_off(uint8_t address, uint8_t channel)
{
    if (channel > 15) {
        return false;
    }

    uint8_t reg = REG_LED0_ON_L + (4 * channel);

    uint8_t data[5];
    data[0] = reg;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = LED_FULL_BIT;

    int result = i2c_write_blocking(pca9685_i2c, address, data, 5, false);

    return result == 5;
}


// ======================================================
// ALL CHANNEL CONTROL
// ======================================================

bool pca9685_all_off(uint8_t address)
{
    uint8_t data[5];

    data[0] = REG_ALL_ON_L;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = LED_FULL_BIT;

    int result = i2c_write_blocking(pca9685_i2c, address, data, 5, false);

    return result == 5;
}

bool pca9685_all_on(uint8_t address)
{
    uint8_t data[5];

    data[0] = REG_ALL_ON_L;
    data[1] = 0x00;
    data[2] = LED_FULL_BIT;
    data[3] = 0x00;
    data[4] = 0x00;

    int result = i2c_write_blocking(pca9685_i2c, address, data, 5, false);

    return result == 5;
}