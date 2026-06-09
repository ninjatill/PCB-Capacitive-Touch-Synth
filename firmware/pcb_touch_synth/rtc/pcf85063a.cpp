#include <stdio.h>

#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "ff.h"

#include "pcf85063a.h"
#include "../config/i2c_addresses.h"

static bool _initialized = false;
static bool _time_valid   = false;

// ---- BCD helpers ----
static inline uint8_t bcd_to_bin(uint8_t bcd) { return (bcd >> 4) * 10 + (bcd & 0x0F); }
static inline uint8_t bin_to_bcd(uint8_t bin)  { return ((bin / 10) << 4) | (bin % 10); }

// ---- Low-level I2C helpers ----

static bool write_reg(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    return i2c_write_blocking(i2c1, I2C_ADDR_RTC, buf, 2, false) == 2;
}

static bool write_regs(uint8_t reg, const uint8_t* data, size_t len)
{
    // Build a single buffer: [reg, d0, d1, ..., dn-1]
    uint8_t buf[16];
    if (len + 1 > sizeof(buf)) return false;
    buf[0] = reg;
    for (size_t i = 0; i < len; i++) buf[i + 1] = data[i];
    return i2c_write_blocking(i2c1, I2C_ADDR_RTC, buf, len + 1, false) == (int)(len + 1);
}

static bool read_regs(uint8_t reg, uint8_t* data, size_t len)
{
    if (i2c_write_blocking(i2c1, I2C_ADDR_RTC, &reg, 1, true) != 1) return false;
    return i2c_read_blocking(i2c1, I2C_ADDR_RTC, data, len, false) == (int)len;
}

// ======================================================
// Public implementation
// ======================================================

bool pcf85063a_init(void)
{
    // Step 1 — verify the device is present and read the OS flag.
    // The OS flag (Seconds register bit 7) is the decision point for
    // whether we issue a software reset.  We must read it FIRST, before
    // touching any control register, so we know whether valid time is held.
    uint8_t seconds_reg;
    if (!read_regs(PCF85063A_REG_SECONDS, &seconds_reg, 1)) {
        printf("PCF85063A: no ACK — device not present on I2C1:0x%02X\n",
               I2C_ADDR_RTC);
        return false;
    }

    const bool os_flag_set = (seconds_reg & PCF85063A_OS_FLAG) != 0;

    // Step 2 — conditional software reset.
    //
    // The datasheet (Section 13.1) recommends a software reset on every MCU
    // boot.  That guidance assumes the RTC VDD also goes to 0 V when the
    // system powers off, making every boot a fresh start.  In this design,
    // CHG_OUT + 1 F supercap keeps the RTC VDD alive through MCU reboots,
    // so the registers remain valid.
    //
    // Rules:
    //   OS flag SET   → RTC lost power or was never initialised.
    //                   Time registers contain reset defaults (garbage).
    //                   Safe to software reset — no valid time to preserve.
    //   OS flag CLEAR → RTC has been running continuously on supercap.
    //                   Time registers hold valid time.
    //                   Do NOT software reset — that would wipe the time.
    //
    // Reference: PCF85063A datasheet Section 7.2.1.3 and Section 13.1.
    if (os_flag_set) {
        uint8_t rst_buf[2] = { PCF85063A_REG_CONTROL_1, PCF85063A_SOFTWARE_RESET };
        if (i2c_write_blocking(i2c1, I2C_ADDR_RTC, rst_buf, 2, false) != 2) {
            printf("PCF85063A: software reset write failed\n");
            return false;
        }
        sleep_ms(2);  // allow internal reset to complete
    }

    // Step 3 — configure Control_1.
    // CAP_SEL=1 must always be set: the hardware POR and software reset both
    // default it to 0 (7 pF), which would mismatch the 12.5 pF Kyocera crystal.
    // When OS was clear we read the current register and OR in the bit to avoid
    // disturbing any other state; when OS was set the reset just cleared everything
    // so we can write the full byte directly.
    uint8_t ctrl1;
    if (os_flag_set) {
        ctrl1 = PCF85063A_CTRL1_CAP_SEL;  // post-reset: write fresh value
    } else {
        if (!read_regs(PCF85063A_REG_CONTROL_1, &ctrl1, 1)) return false;
        ctrl1 |=  PCF85063A_CTRL1_CAP_SEL;
        ctrl1 &= ~(PCF85063A_CTRL1_12_24    // enforce 24-hour mode
                 | PCF85063A_CTRL1_STOP     // ensure oscillator is running
                 | PCF85063A_CTRL1_EXT_TEST);
    }
    if (!write_reg(PCF85063A_REG_CONTROL_1, ctrl1)) {
        printf("PCF85063A: failed to write Control_1\n");
        return false;
    }

    // Step 4 — disable CLKOUT.  Not connected on this board; leaving it
    // enabled wastes ~1–10 µA driving an unloaded push-pull output.
    if (!write_reg(PCF85063A_REG_CONTROL_2, PCF85063A_CTRL2_COF_OFF)) {
        printf("PCF85063A: failed to write Control_2\n");
        return false;
    }

    _time_valid  = !os_flag_set;
    _initialized = true;

    if (_time_valid) {
        printf("PCF85063A: initialised — time valid (RTC was running on supercap)\n");
    } else {
        printf("PCF85063A: initialised — OS flag set, time not set (use 'time set')\n");
    }

    return true;
}

bool pcf85063a_is_valid(void)
{
    return _initialized && _time_valid;
}

bool pcf85063a_get_time(RtcTime* t)
{
    if (!t) return false;

    // Read all 7 time/date registers in one transaction (0x04–0x0A).
    // The chip freezes the time counters during this read to prevent
    // roll-over corruption between individual byte reads (Section 7.4).
    uint8_t raw[7];
    if (!read_regs(PCF85063A_REG_SECONDS, raw, 7)) return false;

    t->valid   = !(raw[0] & PCF85063A_OS_FLAG);
    t->seconds  = bcd_to_bin(raw[0] & 0x7F);
    t->minutes  = bcd_to_bin(raw[1] & 0x7F);
    t->hours    = bcd_to_bin(raw[2] & 0x3F);  // 24-hour mode
    t->day      = bcd_to_bin(raw[3] & 0x3F);
    t->weekday  = raw[4] & 0x07;
    t->month    = bcd_to_bin(raw[5] & 0x1F);
    t->year     = bcd_to_bin(raw[6]);

    return true;
}

bool pcf85063a_set_time(const RtcTime* t)
{
    if (!t || !_initialized) return false;

    // Stop the oscillator divider chain so the time counters are frozen
    // while we write. F0 and F1 (32 kHz prescaler stages) keep running
    // but the 1 Hz tick is suppressed. See datasheet Section 7.2.1.2.
    uint8_t ctrl1 = PCF85063A_CTRL1_CAP_SEL | PCF85063A_CTRL1_STOP;
    if (!write_reg(PCF85063A_REG_CONTROL_1, ctrl1)) return false;

    // Build the 7-byte time block.  The OS flag (bit 7 of Seconds) is
    // written as 0 here, which clears it and marks the time as valid.
    uint8_t raw[7];
    raw[0] = bin_to_bcd(t->seconds);                // OS flag = 0 (time now valid)
    raw[1] = bin_to_bcd(t->minutes);
    raw[2] = bin_to_bcd(t->hours);
    raw[3] = bin_to_bcd(t->day);
    raw[4] = t->weekday & 0x07;
    raw[5] = bin_to_bcd(t->month);
    raw[6] = bin_to_bcd(t->year);

    if (!write_regs(PCF85063A_REG_SECONDS, raw, 7)) {
        // Restart oscillator even on failure
        write_reg(PCF85063A_REG_CONTROL_1, PCF85063A_CTRL1_CAP_SEL);
        return false;
    }

    // Restart oscillator divider chain (clear STOP bit, keep CAP_SEL).
    if (!write_reg(PCF85063A_REG_CONTROL_1, PCF85063A_CTRL1_CAP_SEL)) return false;

    _time_valid = true;
    return true;
}

void pcf85063a_print_status(void)
{
    if (!_initialized) {
        printf("  PCF85063A: not initialised\n");
        return;
    }

    RtcTime t;
    if (!pcf85063a_get_time(&t)) {
        printf("  PCF85063A: I2C read error\n");
        return;
    }

    static const char* wday_names[] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
    const char* wday = (t.weekday <= 6) ? wday_names[t.weekday] : "???";

    printf("  PCF85063A RTC @ I2C1:0x%02X\n", I2C_ADDR_RTC);
    printf("  Time : %02d:%02d:%02d\n", t.hours, t.minutes, t.seconds);
    printf("  Date : 20%02d-%02d-%02d (%s)\n", t.year, t.month, t.day, wday);
    printf("  Valid: %s\n", t.valid ? "yes" : "NO — OS flag set, use 'time set'");
}

// ======================================================
// FatFS timestamp callback
//
// get_fattime() was previously in board/hw_config.c returning a fixed
// date.  Now that an RTC is present it returns the real time, with
// a fallback to 2026-01-01 if the RTC is not initialised or the OS
// flag is set.  The function is declared extern "C" so the C-compiled
// FatFS library can call it directly.
//
// FAT timestamp format (DWORD):
//   Bits 31:25 — year offset from 1980
//   Bits 24:21 — month  (1–12)
//   Bits 20:16 — day    (1–31)
//   Bits 15:11 — hour   (0–23)
//   Bits 10:5  — minute (0–59)
//   Bits  4:0  — second / 2 (0–29)
// ======================================================
extern "C" DWORD get_fattime(void)
{
    RtcTime t;
    if (_initialized && pcf85063a_get_time(&t) && t.valid) {
        return ((DWORD)(t.year + 2000u - 1980u) << 25)
             | ((DWORD)t.month                  << 21)
             | ((DWORD)t.day                    << 16)
             | ((DWORD)t.hours                  << 11)
             | ((DWORD)t.minutes                <<  5)
             | ((DWORD)(t.seconds / 2u));
    }
    // Fallback: fixed epoch so FatFS files get a sane timestamp
    return ((DWORD)(2026u - 1980u) << 25)
         | ((DWORD)1u              << 21)
         | ((DWORD)1u              << 16);
}
