#pragma once

#include <stdint.h>
#include <stdbool.h>

// ======================================================
// PCF85063A — Tiny Real-Time Clock/Calendar
//
// I2C1, address 0x51 (binary 1010001, fixed — no address pins).
// 18 registers, auto-increment. Max I2C clock: 400 kHz.
//
// Crystal: Kyocera DT3215DB32768H5HPWAA
//   32.768 kHz, CL = 12.5 pF, ESR max 50 kΩ.
//   CAP_SEL bit in Control_1 must be set to 1 after every POR
//   so the internal oscillator capacitors present 12.5 pF.
//
// Power: CHG_OUT → MMSD301T1G Schottky → 150 Ω → 1 F supercap → VDD.
//   Backup duration at 25 °C: ~190 days (dominated by RTC standby at 220 nA).
//
// Oscillator stop (OS) flag (Seconds register bit 7):
//   Set at power-on until oscillation stabilises and cleared by software.
//   If set, the time registers contain reset defaults (00:00:00, 2000-01-01)
//   and must be re-set via pcf85063a_set_time() before use.
//
// Time registers use BCD encoding (see Section 7.3 of datasheet).
// Read and write the full 7-byte block (0x04–0x0A) in a single I2C
// transaction to avoid roll-over corruption between individual bytes.
// ======================================================

// ---- Register addresses ----
#define PCF85063A_REG_CONTROL_1   0x00
#define PCF85063A_REG_CONTROL_2   0x01
#define PCF85063A_REG_OFFSET      0x02
#define PCF85063A_REG_RAM_BYTE    0x03
#define PCF85063A_REG_SECONDS     0x04
#define PCF85063A_REG_MINUTES     0x05
#define PCF85063A_REG_HOURS       0x06
#define PCF85063A_REG_DAYS        0x07
#define PCF85063A_REG_WEEKDAYS    0x08
#define PCF85063A_REG_MONTHS      0x09
#define PCF85063A_REG_YEARS       0x0A

// ---- Control_1 bit masks ----
#define PCF85063A_CTRL1_CAP_SEL   (1u << 0)  // 0 = 7 pF (default), 1 = 12.5 pF
#define PCF85063A_CTRL1_12_24     (1u << 1)  // 0 = 24-hour (default), 1 = 12-hour
#define PCF85063A_CTRL1_CIE       (1u << 2)  // correction interrupt enable
#define PCF85063A_CTRL1_SR        (1u << 4)  // software reset (always reads 0)
#define PCF85063A_CTRL1_STOP      (1u << 5)  // stop oscillator divider chain
#define PCF85063A_CTRL1_EXT_TEST  (1u << 7)  // test mode (do not set in production)

// ---- Control_2 CLKOUT field ----
#define PCF85063A_CTRL2_COF_OFF   0x07u      // COF[2:0]=111 disables CLKOUT pin

// ---- Seconds register flag ----
#define PCF85063A_OS_FLAG         (1u << 7)  // oscillator stop flag; clear after verifying time

// ---- Software reset magic byte ----
// Bits 6, 4, 3 set to 1, all others 0: 0101 1000 = 0x58
#define PCF85063A_SOFTWARE_RESET  0x58u

// ---- Weekday encoding (datasheet default; user may reassign) ----
#define PCF85063A_WDAY_SUNDAY     0
#define PCF85063A_WDAY_MONDAY     1
#define PCF85063A_WDAY_TUESDAY    2
#define PCF85063A_WDAY_WEDNESDAY  3
#define PCF85063A_WDAY_THURSDAY   4
#define PCF85063A_WDAY_FRIDAY     5
#define PCF85063A_WDAY_SATURDAY   6

// ======================================================
// RTC time/date structure
// All fields are in natural (binary) units, not BCD.
// The driver converts to/from BCD internally.
// ======================================================
struct RtcTime {
    uint8_t seconds;   // 0–59
    uint8_t minutes;   // 0–59
    uint8_t hours;     // 0–23 (24-hour mode always used)
    uint8_t day;       // 1–31
    uint8_t weekday;   // 0–6  (PCF85063A_WDAY_* constants)
    uint8_t month;     // 1–12
    uint8_t year;      // 0–99 (years since 2000; 26 = 2026)
    bool    valid;     // false if OS flag was set — time not trustworthy
};

// ======================================================
// Public API
// ======================================================

// Initialise the PCF85063A on I2C1.
// Performs a software reset, sets CAP_SEL=1 (12.5 pF), disables CLKOUT,
// and checks the OS flag.  Returns false if the device does not ACK.
bool pcf85063a_init(void);

// Returns true if init succeeded and the OS flag was clear (time is valid).
bool pcf85063a_is_valid(void);

// Read the current time into *t.  t->valid reflects the OS flag state.
// Returns false on I2C error.
bool pcf85063a_get_time(RtcTime* t);

// Set the time.  Stops the oscillator divider, writes all 7 registers in
// one transaction, then restarts.  Also clears the OS flag.
// Returns false on I2C error.
bool pcf85063a_set_time(const RtcTime* t);

// Print decoded time and driver status to stdout (for the debug console).
void pcf85063a_print_status(void);
