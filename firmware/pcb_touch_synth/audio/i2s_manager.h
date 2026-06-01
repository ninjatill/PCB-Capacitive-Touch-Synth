#pragma once

bool i2s_manager_init();

void i2s_manager_start_playback();
void i2s_manager_start_recording();
void i2s_manager_stop();

void i2s_manager_write_silence();