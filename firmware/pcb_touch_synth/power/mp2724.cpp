#include <stdio.h>

#include "mp2724.h"

static i2c_inst_t* mp2724_i2c = nullptr;
static uint8_t mp2724_addr = 0;

// Forward declarations for private helpers defined later in this file.
static bool mp2724_read_register(uint8_t reg, uint8_t* value);
static bool mp2724_write_register(uint8_t reg, uint8_t value);

// ======================================================
// MP2724 REGISTERS
// ======================================================

static constexpr uint8_t REG_IIN       = 0x01;
static constexpr uint8_t REG_CHG_CTRL4 = 0x09;
static constexpr uint8_t REG_VIN_DET   = 0x0A;

static constexpr uint8_t REG_STATUS0   = 0x11;
static constexpr uint8_t REG_STATUS1   = 0x12;
static constexpr uint8_t REG_STATUS2   = 0x13;
static constexpr uint8_t REG_STATUS3   = 0x14;
static constexpr uint8_t REG_STATUS4   = 0x15;
static constexpr uint8_t REG_STATUS5   = 0x16;

// STATUS0
static constexpr uint8_t STATUS0_DPDM_MASK   = 0xF0;
static constexpr uint8_t STATUS0_VINDPM_MASK = 0x02;
static constexpr uint8_t STATUS0_IINDPM_MASK = 0x01;

// STATUS1
static constexpr uint8_t STATUS1_VIN_GD_MASK         = 0x40;
static constexpr uint8_t STATUS1_VIN_RDY_MASK        = 0x20;
static constexpr uint8_t STATUS1_LEGACY_CABLE_MASK   = 0x10;
static constexpr uint8_t STATUS1_THERM_STAT_MASK     = 0x08;
static constexpr uint8_t STATUS1_WATCHDOG_FAULT_MASK = 0x02;
static constexpr uint8_t STATUS1_WATCHDOG_BARK_MASK  = 0x01;

// STATUS2
static constexpr uint8_t STATUS2_CHG_STAT_MASK   = 0xE0;
static constexpr uint8_t STATUS2_BOOST_FAULT_MASK = 0x1C;
static constexpr uint8_t STATUS2_CHG_FAULT_MASK   = 0x03;

// STATUS4
static constexpr uint8_t STATUS4_CC1_MASK = 0xC0;
static constexpr uint8_t STATUS4_CC2_MASK = 0x30;

// STATUS5
static constexpr uint8_t STATUS5_BFET_STAT_MASK = 0x20;
static constexpr uint8_t STATUS5_BATT_LOW_MASK  = 0x10;


// ======================================================
// LOW-LEVEL I2C
// ======================================================

bool mp2724_init(i2c_inst_t* i2c, uint8_t address)
{
    printf("Initializing MP2724 driver...\n");

    mp2724_i2c = i2c;
    mp2724_addr = address;

    uint8_t status1 = 0;

    if (!mp2724_read_register(REG_STATUS1, &status1)) {
        printf("MP2724 not detected at address 0x%02X\n", mp2724_addr);
        return false;
    }

    printf("MP2724 detected at address 0x%02X\n", mp2724_addr);
    printf("MP2724 STATUS1 = 0x%02X\n", status1);

    return true;
}

static bool mp2724_read_register(uint8_t reg, uint8_t* value)
{
    if (mp2724_i2c == nullptr || value == nullptr) {
        return false;
    }

    int result = i2c_write_blocking(
        mp2724_i2c,
        mp2724_addr,
        &reg,
        1,
        true
    );

    if (result != 1) {
        printf("MP2724 register select failed: reg=0x%02X\n", reg);
        return false;
    }

    result = i2c_read_blocking(
        mp2724_i2c,
        mp2724_addr,
        value,
        1,
        false
    );

    if (result != 1) {
        printf("MP2724 read failed: reg=0x%02X\n", reg);
        return false;
    }

    return true;
}

static bool mp2724_write_register(uint8_t reg, uint8_t value)
{
    if (mp2724_i2c == nullptr) {
        return false;
    }

    uint8_t data[2] = { reg, value };

    int result = i2c_write_blocking(
        mp2724_i2c,
        mp2724_addr,
        data,
        2,
        false
    );

    if (result != 2) {
        printf("MP2724 write failed: reg=0x%02X value=0x%02X\n",
               reg,
               value);
        return false;
    }

    return true;
}


// ======================================================
// DECODE HELPERS
// ======================================================

static Mp2724DpdmStatus decode_dpdm(uint8_t raw)
{
    switch ((raw & STATUS0_DPDM_MASK) >> 4) {
        case 0x0: return MP2724_DPDM_NOT_STARTED;
        case 0x1: return MP2724_DPDM_USB_SDP;
        case 0x2: return MP2724_DPDM_USB_DCP;
        case 0x3: return MP2724_DPDM_USB_CDP;
        case 0x4: return MP2724_DPDM_DIVIDER_1;
        case 0x5: return MP2724_DPDM_DIVIDER_2;
        case 0x6: return MP2724_DPDM_DIVIDER_3;
        case 0x7: return MP2724_DPDM_DIVIDER_4;
        case 0x8: return MP2724_DPDM_UNKNOWN;
        case 0x9: return MP2724_DPDM_USB_DCP;
        case 0xE: return MP2724_DPDM_DIVIDER_5;
        default:  return MP2724_DPDM_UNKNOWN;
    }
}

static Mp2724CcStatus decode_cc(uint8_t bits)
{
    switch (bits & 0x03) {
        case 0x0: return MP2724_CC_VRA;
        case 0x1: return MP2724_CC_DEFAULT_USB;
        case 0x2: return MP2724_CC_1P5A;
        case 0x3: return MP2724_CC_3A;
        default:  return MP2724_CC_VRA;
    }
}

static bool dpdm_is_high_power(Mp2724DpdmStatus status)
{
    switch (status) {
        case MP2724_DPDM_USB_DCP:
        case MP2724_DPDM_USB_CDP:
        case MP2724_DPDM_DIVIDER_1:
        case MP2724_DPDM_DIVIDER_2:
        case MP2724_DPDM_DIVIDER_3:
        case MP2724_DPDM_DIVIDER_4:
        case MP2724_DPDM_DIVIDER_5:
            return true;

        default:
            return false;
    }
}

static bool cc_is_high_power(Mp2724CcStatus status)
{
    return status == MP2724_CC_1P5A ||
           status == MP2724_CC_3A;
}


// ======================================================
// STATUS READ
// ======================================================

bool mp2724_read_status(Mp2724Status* status)
{
    if (status == nullptr) {
        return false;
    }

    if (!mp2724_read_register(REG_STATUS0, &status->status0)) return false;
    if (!mp2724_read_register(REG_STATUS1, &status->status1)) return false;
    if (!mp2724_read_register(REG_STATUS2, &status->status2)) return false;
    if (!mp2724_read_register(REG_STATUS3, &status->status3)) return false;
    if (!mp2724_read_register(REG_STATUS4, &status->status4)) return false;
    if (!mp2724_read_register(REG_STATUS5, &status->status5)) return false;

    status->vin_good = (status->status1 & STATUS1_VIN_GD_MASK) != 0;
    status->vin_ready = (status->status1 & STATUS1_VIN_RDY_MASK) != 0;
    status->legacy_cable = (status->status1 & STATUS1_LEGACY_CABLE_MASK) != 0;
    status->thermal_regulation = (status->status1 & STATUS1_THERM_STAT_MASK) != 0;
    status->watchdog_fault = (status->status1 & STATUS1_WATCHDOG_FAULT_MASK) != 0;
    status->watchdog_bark = (status->status1 & STATUS1_WATCHDOG_BARK_MASK) != 0;

    status->dpdm_status = decode_dpdm(status->status0);

    status->cc1_status = decode_cc((status->status4 & STATUS4_CC1_MASK) >> 6);
    status->cc2_status = decode_cc((status->status4 & STATUS4_CC2_MASK) >> 4);

    status->in_vindpm = (status->status0 & STATUS0_VINDPM_MASK) != 0;
    status->in_iindpm = (status->status0 & STATUS0_IINDPM_MASK) != 0;

    status->charge_status = (status->status2 & STATUS2_CHG_STAT_MASK) >> 5;
    status->boost_fault = (status->status2 & STATUS2_BOOST_FAULT_MASK) >> 2;
    status->charge_fault = status->status2 & STATUS2_CHG_FAULT_MASK;

    status->battery_discharging = (status->status5 & STATUS5_BFET_STAT_MASK) != 0;
    status->batt_low = (status->status5 & STATUS5_BATT_LOW_MASK) != 0;

    return true;
}


// ======================================================
// POWER SOURCE / CAPABILITY
// ======================================================

bool mp2724_get_power_source(Mp2724PowerSource* source)
{
    if (source == nullptr) {
        return false;
    }

    Mp2724Status status;

    if (!mp2724_read_status(&status)) {
        return false;
    }

    if (!status.vin_good) {
        *source = MP2724_POWER_SOURCE_DISCONNECTED;
        return true;
    }

    if (!status.vin_ready) {
        *source = MP2724_POWER_SOURCE_DETECTING;
        return true;
    }

    if (cc_is_high_power(status.cc1_status) ||
        cc_is_high_power(status.cc2_status)) {
        *source = MP2724_POWER_SOURCE_USB_HIGH_CURRENT;
        return true;
    }

    if (dpdm_is_high_power(status.dpdm_status)) {
        *source = MP2724_POWER_SOURCE_ADAPTER;
        return true;
    }

    if (status.dpdm_status == MP2724_DPDM_USB_SDP ||
        status.dpdm_status == MP2724_DPDM_NOT_STARTED ||
        status.dpdm_status == MP2724_DPDM_UNKNOWN) {
        *source = MP2724_POWER_SOURCE_USB_500MA;
        return true;
    }

    *source = MP2724_POWER_SOURCE_UNKNOWN;
    return true;
}

bool mp2724_high_power_available(bool* available)
{
    if (available == nullptr) {
        return false;
    }

    Mp2724PowerSource source;

    if (!mp2724_get_power_source(&source)) {
        return false;
    }

    *available =
        source == MP2724_POWER_SOURCE_USB_HIGH_CURRENT ||
        source == MP2724_POWER_SOURCE_ADAPTER;

    return true;
}

bool mp2724_input_present(bool* present)
{
    if (present == nullptr) {
        return false;
    }

    Mp2724Status status;

    if (!mp2724_read_status(&status)) {
        return false;
    }

    *present = status.vin_good;
    return true;
}

bool mp2724_power_ready(bool* ready)
{
    if (ready == nullptr) {
        return false;
    }

    Mp2724Status status;

    if (!mp2724_read_status(&status)) {
        return false;
    }

    *ready = status.vin_good && status.vin_ready;
    return true;
}

// ======================================================
// SET POWER CONFIGURATION
// ======================================================
bool mp2724_enable_usb_c_sink_detection()
{
    uint8_t reg = 0;

    if (!mp2724_read_register(0x09, &reg)) {
        return false;
    }

    // CHG_CTRL4 / 0x09
    // Bits 6:4 = CC_CFG
    // 000 = enable CC1/CC2 sink mode
    reg &= ~(0b111 << 4);

    return mp2724_write_register(0x09, reg);
}

bool mp2724_force_dpdm_detection()
{
    uint8_t reg = 0;

    if (!mp2724_read_register(0x0A, &reg)) {
        return false;
    }

    // VIN_DET / 0x0A
    // Bit 5 AUTODPDM should stay enabled.
    // Bit 4 FORCEDPDM = 1 restarts D+/D- detection, self-clears.
    reg |= (1 << 5);  // AUTODPDM
    reg |= (1 << 4);  // FORCEDPDM

    return mp2724_write_register(0x0A, reg);
}


// ======================================================
// STRING HELPERS
// ======================================================

const char* mp2724_power_source_name(Mp2724PowerSource source)
{
    switch (source) {
        case MP2724_POWER_SOURCE_DISCONNECTED:      return "Disconnected";
        case MP2724_POWER_SOURCE_DETECTING:         return "Detecting";
        case MP2724_POWER_SOURCE_USB_500MA:         return "USB 500mA";
        case MP2724_POWER_SOURCE_USB_HIGH_CURRENT:  return "USB-C high current";
        case MP2724_POWER_SOURCE_ADAPTER:           return "Adapter / charger";
        case MP2724_POWER_SOURCE_UNKNOWN:           return "Unknown";
        default:                                    return "Invalid";
    }
}

const char* mp2724_dpdm_status_name(Mp2724DpdmStatus status)
{
    switch (status) {
        case MP2724_DPDM_NOT_STARTED: return "Not started / 500mA";
        case MP2724_DPDM_USB_SDP:     return "USB SDP / 500mA";
        case MP2724_DPDM_USB_DCP:     return "USB DCP / 2A";
        case MP2724_DPDM_USB_CDP:     return "USB CDP / 1.5A";
        case MP2724_DPDM_DIVIDER_1:   return "Divider 1 / 1A";
        case MP2724_DPDM_DIVIDER_2:   return "Divider 2 / 2.1A";
        case MP2724_DPDM_DIVIDER_3:   return "Divider 3 / 2.4A";
        case MP2724_DPDM_DIVIDER_4:   return "Divider 4 / 2A";
        case MP2724_DPDM_UNKNOWN:     return "Unknown / 500mA";
        case MP2724_DPDM_DIVIDER_5:   return "Divider 5 / 3A";
        default:                      return "Invalid";
    }
}

const char* mp2724_cc_status_name(Mp2724CcStatus status)
{
    switch (status) {
        case MP2724_CC_VRA:          return "vRa";
        case MP2724_CC_DEFAULT_USB:  return "Default USB";
        case MP2724_CC_1P5A:         return "1.5A";
        case MP2724_CC_3A:           return "3A";
        default:                     return "Invalid";
    }
}


// ======================================================
// DEBUG PRINT
// ======================================================

void mp2724_print_status()
{
    Mp2724Status status;
    Mp2724PowerSource source;

    if (!mp2724_read_status(&status)) {
        printf("MP2724 status read failed.\n");
        return;
    }

    if (!mp2724_get_power_source(&source)) {
        source = MP2724_POWER_SOURCE_UNKNOWN;
    }

    printf("MP2724 status:\n");
    printf("  STATUS0 = 0x%02X\n", status.status0);
    printf("  STATUS1 = 0x%02X\n", status.status1);
    printf("  STATUS2 = 0x%02X\n", status.status2);
    printf("  STATUS3 = 0x%02X\n", status.status3);
    printf("  STATUS4 = 0x%02X\n", status.status4);
    printf("  STATUS5 = 0x%02X\n", status.status5);

    printf("  VIN good      = %s\n", status.vin_good ? "yes" : "no");
    printf("  VIN ready     = %s\n", status.vin_ready ? "yes" : "no");
    printf("  legacy cable  = %s\n", status.legacy_cable ? "yes" : "no");

    printf("  DPDM status   = %s\n", mp2724_dpdm_status_name(status.dpdm_status));
    printf("  CC1 status    = %s\n", mp2724_cc_status_name(status.cc1_status));
    printf("  CC2 status    = %s\n", mp2724_cc_status_name(status.cc2_status));
    printf("  power source  = %s\n", mp2724_power_source_name(source));

    printf("  VINDPM        = %s\n", status.in_vindpm ? "yes" : "no");
    printf("  IINDPM        = %s\n", status.in_iindpm ? "yes" : "no");
    printf("  thermal reg   = %s\n", status.thermal_regulation ? "yes" : "no");
    printf("  batt low      = %s\n", status.batt_low ? "yes" : "no");
    printf("  batt dischg   = %s\n", status.battery_discharging ? "yes" : "no");

    printf("  charge stat   = %u\n", status.charge_status);
    printf("  charge fault  = %u\n", status.charge_fault);
    printf("  boost fault   = %u\n", status.boost_fault);
}

