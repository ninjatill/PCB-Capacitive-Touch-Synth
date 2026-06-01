#pragma once

#include <stdint.h>
#include "hardware/i2c.h"

bool tlv320dac3100_init(i2c_inst_t* i2c, uint8_t address);

bool tlv320dac3100_select_page(uint8_t page);
bool tlv320dac3100_read_register(uint8_t page, uint8_t reg, uint8_t* value);
bool tlv320dac3100_write_register(uint8_t page, uint8_t reg, uint8_t value);
bool tlv320dac3100_write_registers(uint8_t page, uint8_t start_reg, const uint8_t* data, uint8_t length);

bool tlv320dac3100_soft_reset();

bool tlv320dac3100_set_dac_volume_db(float db);
bool tlv320dac3100_mute_dac(bool mute);

bool tlv320dac3100_set_headphone_volume_db(float db);
bool tlv320dac3100_set_speaker_volume_db(float db);

bool tlv320dac3100_enable_headphone_detect();
bool tlv320dac3100_headphone_inserted(bool* inserted);

bool tlv320dac3100_play_beep_1khz();
void tlv320dac3100_print_basic_status();