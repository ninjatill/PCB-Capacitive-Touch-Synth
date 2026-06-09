#pragma once

// ======================================================
// MP2724 USB-C SINK CONTROLLER / BATTERY CHARGER DRIVER
//
// The MP2724 handles:
//   - USB-C CC line detection (current advertisement via CC1/CC2)
//   - Legacy D+/D- (DPDM) charger detection (BC1.2 / Apple divider)
//   - Li-ion battery charging
//   - Power path management (input vs. battery)
//
// I2C bus:     I2C1 (PIN_I2C1_SDA / PIN_I2C1_SCL), 400kHz
// I2C address: I2C_ADDR_MP2724
// IRQ pin:     PIN_CHARGER_IRQ (active-low, edge-triggered)
//
// After power_startup() completes, the firmware uses mp2724_high_power_available()
// to decide whether to enable the 5V boost rail (PIN_5V_EN).
// ======================================================

#include <stdint.h>
#include "hardware/i2c.h"

enum Mp2724DpdmStatus {
    MP2724_DPDM_NOT_STARTED,
    MP2724_DPDM_USB_SDP,
    MP2724_DPDM_USB_DCP,
    MP2724_DPDM_USB_CDP,
    MP2724_DPDM_DIVIDER_1,
    MP2724_DPDM_DIVIDER_2,
    MP2724_DPDM_DIVIDER_3,
    MP2724_DPDM_DIVIDER_4,
    MP2724_DPDM_UNKNOWN,
    MP2724_DPDM_DIVIDER_5
};

enum Mp2724CcStatus {
    MP2724_CC_VRA,
    MP2724_CC_DEFAULT_USB,
    MP2724_CC_1P5A,
    MP2724_CC_3A
};

enum Mp2724PowerSource {
    MP2724_POWER_SOURCE_DISCONNECTED,
    MP2724_POWER_SOURCE_DETECTING,
    MP2724_POWER_SOURCE_USB_500MA,
    MP2724_POWER_SOURCE_USB_HIGH_CURRENT,
    MP2724_POWER_SOURCE_ADAPTER,
    MP2724_POWER_SOURCE_UNKNOWN
};

struct Mp2724Status {
    uint8_t status0;
    uint8_t status1;
    uint8_t status2;
    uint8_t status3;
    uint8_t status4;
    uint8_t status5;

    bool vin_good;
    bool vin_ready;
    bool legacy_cable;
    bool thermal_regulation;
    bool watchdog_fault;
    bool watchdog_bark;

    Mp2724DpdmStatus dpdm_status;
    Mp2724CcStatus cc1_status;
    Mp2724CcStatus cc2_status;

    bool in_vindpm;
    bool in_iindpm;
    bool batt_low;
    bool battery_discharging;

    uint8_t charge_status;
    uint8_t charge_fault;
    uint8_t boost_fault;
};

bool mp2724_init(i2c_inst_t* i2c, uint8_t address);

bool mp2724_read_status(Mp2724Status* status);

bool mp2724_get_power_source(Mp2724PowerSource* source);
bool mp2724_high_power_available(bool* available);
bool mp2724_input_present(bool* present);
bool mp2724_power_ready(bool* ready);

bool mp2724_enable_usb_c_sink_detection();
bool mp2724_force_dpdm_detection();

// Write REG_IIN (0x01) input current limit.
// Encoding: bits [6:0] = mA / 50  (50 mA steps, range 50–3 250 mA).
// Called after source detection so the chip enforces USB compliance.
// The MP2724 automatically supplements from the battery when the system
// load exceeds the USB limit — but power mode decisions ignore that
// supplementation so the mode stays consistent regardless of battery charge.
bool mp2724_set_input_current_limit_ma(uint16_t ma);

const char* mp2724_power_source_name(Mp2724PowerSource source);
const char* mp2724_dpdm_status_name(Mp2724DpdmStatus status);
const char* mp2724_cc_status_name(Mp2724CcStatus status);

void mp2724_print_status();