#include <stdio.h>

#include "pico/stdlib.h"
#include "tlv320dac3100.h"

static i2c_inst_t* tlv_i2c = nullptr;
static uint8_t tlv_addr = 0;
static uint8_t current_page = 0xFF;

// ======================================================
// TLV320DAC3100 REGISTER MAP (referenced pages/registers)
// All register numbers are within-page addresses (0x00-0x7F).
// Page is selected by writing to register 0x00 on the current page.
// ======================================================

// Common to all pages
constexpr uint8_t REG_PAGE_SELECT      = 0x00;

// Page 0 — clock, interface, DAC, interrupts
constexpr uint8_t REG_SOFTWARE_RESET   = 0x01;
constexpr uint8_t REG_CLOCK_MUX        = 0x04;   // CODEC_CLKIN and PLL_CLKIN source
constexpr uint8_t REG_PLL_P_R          = 0x05;   // PLL power, P divider, R multiplier
constexpr uint8_t REG_PLL_J            = 0x06;   // PLL J multiplier
constexpr uint8_t REG_PLL_D_MSB        = 0x07;   // PLL fractional D (MSB, write first)
constexpr uint8_t REG_PLL_D_LSB        = 0x08;   // PLL fractional D (LSB, write immediately after)
constexpr uint8_t REG_NDAC             = 0x0B;   // NDAC divider value and power
constexpr uint8_t REG_MDAC             = 0x0C;   // MDAC divider value and power
constexpr uint8_t REG_DOSR_MSB         = 0x0D;   // DOSR[9:8]
constexpr uint8_t REG_DOSR_LSB         = 0x0E;   // DOSR[7:0]
constexpr uint8_t REG_IFACE_CTRL1      = 0x1B;   // I2S mode, word length, BCLK/WCLK direction
constexpr uint8_t REG_GPIO1_CTRL       = 0x33;   // GPIO1 function (used as INT1 output)
constexpr uint8_t REG_PROC_BLOCK       = 0x3C;   // DAC processing block selection
constexpr uint8_t REG_DAC_PATH         = 0x3F;   // DAC power, data routing, soft-step
constexpr uint8_t REG_DAC_MUTE_CTRL    = 0x40;   // DAC mute and channel volume ganging
constexpr uint8_t REG_DAC_L_VOL        = 0x41;   // DAC left digital volume
constexpr uint8_t REG_DAC_R_VOL        = 0x42;   // DAC right digital volume
constexpr uint8_t REG_HEADSET_DETECT   = 0x43;   // Headset detection enable and debounce
constexpr uint8_t REG_INT1_CTRL        = 0x30;   // INT1 interrupt source selection
constexpr uint8_t REG_OT_FLAG          = 0x03;   // Over-temperature flag (debug)
constexpr uint8_t REG_DAC_FLAG_1       = 0x25;   // DAC/HP/SPK power flags (debug)
constexpr uint8_t REG_STICKY_FLAGS_1   = 0x2C;   // Sticky interrupt flags (debug)
constexpr uint8_t REG_INT_FLAGS_2      = 0x2E;   // Instantaneous interrupt flags (headphone detect)

// Page 0 — beep generator (PRB_P25 only)
constexpr uint8_t REG_BEEP_L_VOL       = 0x47;
constexpr uint8_t REG_BEEP_R_VOL       = 0x48;
constexpr uint8_t REG_BEEP_LEN_MSB     = 0x49;
constexpr uint8_t REG_BEEP_LEN_MID     = 0x4A;
constexpr uint8_t REG_BEEP_LEN_LSB     = 0x4B;
constexpr uint8_t REG_BEEP_SIN_MSB     = 0x4C;
constexpr uint8_t REG_BEEP_SIN_LSB     = 0x4D;
constexpr uint8_t REG_BEEP_COS_MSB     = 0x4E;
constexpr uint8_t REG_BEEP_COS_LSB     = 0x4F;

// Page 1 — analog outputs and routing
constexpr uint8_t REG_HP_DRIVER_CTRL   = 0x1F;   // HPL/HPR power, common-mode voltage
constexpr uint8_t REG_SPK_DRIVER_CTRL  = 0x20;   // Class-D speaker power
constexpr uint8_t REG_HP_DEPOP         = 0x21;   // HP de-pop power-on time and ramp step
constexpr uint8_t REG_OUTPUT_ROUTING   = 0x23;   // DAC_L / DAC_R to mixer routing
constexpr uint8_t REG_HPL_ANALOG_VOL   = 0x24;   // HPL analog volume + routing enable
constexpr uint8_t REG_HPR_ANALOG_VOL   = 0x25;   // HPR analog volume + routing enable
constexpr uint8_t REG_SPK_ANALOG_VOL   = 0x26;   // Class-D analog volume + routing enable
constexpr uint8_t REG_HPL_DRIVER       = 0x28;   // HPL driver gain and mute
constexpr uint8_t REG_HPR_DRIVER       = 0x29;   // HPR driver gain and mute
constexpr uint8_t REG_SPK_GAIN         = 0x2A;   // Class-D gain (6/12/18/24 dB) and mute


// ======================================================
// LOW-LEVEL I2C
// ======================================================

static bool tlv320dac3100_select_page(uint8_t page)
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

static bool tlv320dac3100_write_register(uint8_t page, uint8_t reg, uint8_t value)
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

static bool tlv320dac3100_write_registers(uint8_t page, uint8_t start_reg, const uint8_t* data, uint8_t length)
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

static bool tlv320dac3100_read_register(uint8_t page, uint8_t reg, uint8_t* value)
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
        printf("TLV320DAC3100: soft reset failed.\n");
        return false;
    }

    // --------------------------------------------------------
    // STEP 1: Clock tree
    //
    // MCLK pin is unconnected on this board. The TLV320DAC3100
    // defaults to MCLK as CODEC_CLKIN after reset, so we must
    // explicitly switch to the PLL driven by BCLK.
    //
    // The ICS-43432 microphone requires exactly 64 SCK cycles per WS frame,
    // so the RP2040 drives BCLK at AUDIO_SAMPLE_RATE_HZ × 64 (32-bit slots).
    // BCLK = 44100 × 64 = 2,822,400 Hz
    //
    // PLL: P=1, R=1, J=32, D=0
    //   PLL_CLK = BCLK × R × J / P = 2,822,400 × 1 × 32 = 90,316,800 Hz
    //
    // Clock dividers: NDAC=4, MDAC=8, DOSR=64
    //   DAC_fS = PLL_CLK / (NDAC × MDAC × DOSR)
    //          = 90,316,800 / 2048 = 44,100 Hz
    //
    // MDAC × DOSR / 32 = 16 → supports all processing blocks (max RC=12)
    //
    // ICS-43432 SCK check: 0.460 MHz ≤ 2.822 MHz ≤ 3.379 MHz ✓
    // ICS-43432 WS check:  7.19 kHz ≤ 44.1 kHz ≤ 52.8 kHz ✓
    // --------------------------------------------------------

    // Page 0, Reg 4: PLL_CLKIN = BCLK (D3:D2=01), CODEC_CLKIN = PLL_CLK (D1:D0=11)
    if (!tlv320dac3100_write_register(0, REG_CLOCK_MUX, 0x07)) return false;

    // Page 0, Reg 6: J=32 (D5:D0 = 100000) → 0x20
    if (!tlv320dac3100_write_register(0, REG_PLL_J, 0x20)) return false;

    // Page 0, Reg 7 then Reg 8: D=0 (must write 7 then 8 back-to-back)
    if (!tlv320dac3100_write_register(0, REG_PLL_D_MSB, 0x00)) return false;
    if (!tlv320dac3100_write_register(0, REG_PLL_D_LSB, 0x00)) return false;

    // Page 0, Reg 5: PLL power up (D7=1), P=1 (D6:D4=001), R=1 (D3:D0=0001) → 0x91
    if (!tlv320dac3100_write_register(0, REG_PLL_P_R, 0x91)) return false;

    // PLL takes up to 10ms to lock after power-up.
    sleep_ms(15);

    // Page 0, Reg 11: NDAC=4 powered up (D7=1, D6:D0=0000100) → 0x84
    if (!tlv320dac3100_write_register(0, REG_NDAC, 0x84)) return false;

    // Page 0, Reg 12: MDAC=8 powered up (D7=1, D6:D0=0001000) → 0x88
    if (!tlv320dac3100_write_register(0, REG_MDAC, 0x88)) return false;

    // Page 0, Reg 13-14: DOSR=64 (0x0040). Write 13 then 14 back-to-back.
    if (!tlv320dac3100_write_register(0, REG_DOSR_MSB, 0x00)) return false;
    if (!tlv320dac3100_write_register(0, REG_DOSR_LSB, 0x40)) return false;

    // --------------------------------------------------------
    // STEP 2: Audio serial interface
    // I2S mode, 32-bit word length, BCLK and WCLK are inputs (DAC is slave).
    // 32-bit slots are required so the frame has 64 BCLK cycles, matching
    // the ICS-43432 microphone on the shared bus. Audio samples are 24-bit;
    // the upper 8 bits of each DAC word are zero-padded.
    // Page 0, Reg 27: D7:D6=00 (I2S), D5:D4=11 (32-bit), D3=0 (BCLK in), D2=0 (WCLK in)
    // --------------------------------------------------------
    if (!tlv320dac3100_write_register(0, REG_IFACE_CTRL1, 0x30)) return false;

    // --------------------------------------------------------
    // STEP 3: Processing block
    // PRB_P25: Filter A, stereo, DRC, 3D, beep generator, RC=12.
    // MDAC × DOSR / 32 = 16 ≥ 12 ✓
    // --------------------------------------------------------
    if (!tlv320dac3100_write_register(0, REG_PROC_BLOCK, 0x19)) return false;

    // --------------------------------------------------------
    // STEP 4: Headphone detection and GPIO1 / INT1 interrupt
    // GPIO1 is wired to PIN_AUDIO_IRQ on the RP2040.
    // --------------------------------------------------------

    // Page 0, Reg 51: GPIO1 = INT1 output (D5:D2=0101) → 0x14
    if (!tlv320dac3100_write_register(0, REG_GPIO1_CTRL, 0x14)) return false;

    // Page 0, Reg 48: headset-insertion event → INT1 (D7=1), single pulse (D0=0) → 0x80
    if (!tlv320dac3100_write_register(0, REG_INT1_CTRL, 0x80)) return false;

    // Headset detection: enabled (D7=1), 64ms debounce (D4:D2=010), 0ms button debounce
    if (!tlv320dac3100_enable_headphone_detect()) return false;

    // --------------------------------------------------------
    // STEP 5: Analog routing and output driver setup (Page 1)
    //
    // Default output: Class-D speaker. HP drivers powered down.
    // Routing: DAC_L → left mixer → speaker.
    //          DAC_L/R pre-wired to HP drivers too, so switching
    //          is just a matter of powering the correct driver.
    // --------------------------------------------------------

    // Page 1, Reg 31: HPL/HPR powered down, CM=1.65V (D4:D3=10), D2 reserved=1 → 0x14
    if (!tlv320dac3100_write_register(1, REG_HP_DRIVER_CTRL, 0x14)) return false;

    // Page 1, Reg 33: HP de-pop power-on time=304ms (D6:D3=0111), ramp-step=3.9ms (D2:D1=11) → 0x3E
    if (!tlv320dac3100_write_register(1, REG_HP_DEPOP, 0x3E)) return false;

    // Page 1, Reg 35: DAC_L → left mixer (D7:D6=01), DAC_R → right mixer (D3:D2=01) → 0x44
    if (!tlv320dac3100_write_register(1, REG_OUTPUT_ROUTING, 0x44)) return false;

    // Page 1, Reg 36: HPL analog vol — routed to HPL driver (D7=1), 0dB attenuation → 0x80
    if (!tlv320dac3100_write_register(1, REG_HPL_ANALOG_VOL, 0x80)) return false;

    // Page 1, Reg 37: HPR analog vol — routed to HPR driver (D7=1), 0dB attenuation → 0x80
    if (!tlv320dac3100_write_register(1, REG_HPR_ANALOG_VOL, 0x80)) return false;

    // Page 1, Reg 38: speaker analog vol — routed to Class-D (D7=1), 0dB attenuation → 0x80
    if (!tlv320dac3100_write_register(1, REG_SPK_ANALOG_VOL, 0x80)) return false;

    // Page 1, Reg 40: HPL driver 0dB gain (D6:D3=0000), unmuted (D2=1), D1 reserved=1 → 0x06
    if (!tlv320dac3100_write_register(1, REG_HPL_DRIVER, 0x06)) return false;

    // Page 1, Reg 41: HPR driver 0dB gain, unmuted → 0x06
    if (!tlv320dac3100_write_register(1, REG_HPR_DRIVER, 0x06)) return false;

    // Page 1, Reg 42: Class-D gain=6dB (D4:D3=00), not muted (D2=1) → 0x04
    if (!tlv320dac3100_write_register(1, REG_SPK_GAIN, 0x04)) return false;

    // Power up Class-D speaker (default output).
    // Page 1, Reg 32: D7=1 (powered up), D6:D1=000011 (reset value) → 0x86
    if (!tlv320dac3100_write_register(1, REG_SPK_DRIVER_CTRL, 0x86)) return false;

    // Wait for soft-start / de-pop ramp.
    sleep_ms(10);

    // --------------------------------------------------------
    // STEP 6: Power up DAC (Page 0)
    //
    // The DAC must be powered up after all clock and analog
    // configuration is complete. Do not power up during the
    // 1ms initialization lockout after reset.
    // --------------------------------------------------------

    // Page 0, Reg 63: L-DAC up (D7=1), R-DAC up (D6=1),
    //   L→left data (D5:D4=01), R→right data (D3:D2=01),
    //   soft-step 1 step/sample (D1:D0=00) → 0xD4
    if (!tlv320dac3100_write_register(0, REG_DAC_PATH, 0xD4)) return false;

    // Page 0, Reg 64: L and R DAC not muted, independent volume → 0x00
    if (!tlv320dac3100_write_register(0, REG_DAC_MUTE_CTRL, 0x00)) return false;

    // Page 0, Reg 65/66: DAC digital volume = 0dB
    if (!tlv320dac3100_write_register(0, REG_DAC_L_VOL, 0x00)) return false;
    if (!tlv320dac3100_write_register(0, REG_DAC_R_VOL, 0x00)) return false;

    printf("TLV320DAC3100 initialized. fS=44100Hz, speaker default.\n");
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
// OUTPUT SWITCHING
// ======================================================

bool tlv320dac3100_enable_speaker()
{
    printf("TLV320DAC3100: switching to speaker output.\n");

    // Power down HP drivers before enabling Class-D.
    // Page 1 / Reg 31: HPL/HPR down, CM=1.65V (D4:D3=10), D2 reserved=1 → 0x14
    if (!tlv320dac3100_write_register(1, REG_HP_DRIVER_CTRL, 0x14)) return false;

    sleep_ms(2);

    // Page 1 / Reg 32: Class-D powered up, D6:D1=000011 (reset value) → 0x86
    return tlv320dac3100_write_register(1, REG_SPK_DRIVER_CTRL, 0x86);
}

bool tlv320dac3100_enable_headphones()
{
    printf("TLV320DAC3100: switching to headphone output.\n");

    // Power down Class-D before enabling HP drivers.
    // Page 1 / Reg 32: Class-D powered down, D6:D1=000011 → 0x06
    if (!tlv320dac3100_write_register(1, REG_SPK_DRIVER_CTRL, 0x06)) return false;

    sleep_ms(2);

    // Page 1 / Reg 31: HPL up (D7=1), HPR up (D6=1), CM=1.65V (D4:D3=10), D2 reserved=1 → 0xD4
    // De-pop soft-start runs automatically per REG_HP_DEPOP configuration.
    return tlv320dac3100_write_register(1, REG_HP_DRIVER_CTRL, 0xD4);
}


// ======================================================
// HEADPHONE DETECTION
// ======================================================

bool tlv320dac3100_enable_headphone_detect()
{
    // Page 0 / Reg 67:
    // D7=1: headset detection enabled.
    // D4:D2=010: 64ms debounce (avoids false triggers from connector vibration).
    // D1:D0=00: 0ms button-press debounce.
    // Value: 1_00_010_00 = 0x88
    return tlv320dac3100_write_register(0, REG_HEADSET_DETECT, 0x88);
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

    // Page 0 / Reg 46 (REG_INT_FLAGS_2), bit D4:
    //   0 = headset removal detected
    //   1 = headset insertion detected
    *inserted = (flags & 0x10) != 0;

    return true;
}


// ======================================================
// BEEP / KEY-CLICK
// ======================================================

bool tlv320dac3100_play_beep_1khz()
{
    printf("TLV320DAC3100 configuring 1 kHz beep...\n");

    // Beep coefficients for 1 kHz at 44100 Hz sample rate, 5 cycles.
    // MATLAB: Sine   = round(sin(2*pi*1000/44100) * 32768) = 4659 = 0x1233
    //         Cosine = round(cos(2*pi*1000/44100) * 32768) = 32435 = 0x7EB3
    //         Length = floor(44100 * 5 / 1000) = 220 = 0x0000DC
    uint8_t beep_data[] = {
        0x00, // Reg 73 length MSB
        0x00, // Reg 74 length MID
        0xDC, // Reg 75 length LSB  (220 samples = 5 cycles at 44100 Hz)
        0x12, // Reg 76 sine MSB
        0x33, // Reg 77 sine LSB
        0x7E, // Reg 78 cosine MSB
        0xB3  // Reg 79 cosine LSB
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
// DEBUG — CONFIGURATION READBACK
// ======================================================

void tlv320dac3100_print_config()
{
    uint8_t r4, r5, r6, r11, r12, r14, r27, r37, r67, r3c;
    uint8_t p1r31, p1r32, p1r42;

    printf("TLV320DAC3100 configuration (register readback):\n");

    bool ok = true;
    ok &= tlv320dac3100_read_register(0, REG_CLOCK_MUX,      &r4);
    ok &= tlv320dac3100_read_register(0, REG_PLL_P_R,        &r5);
    ok &= tlv320dac3100_read_register(0, REG_PLL_J,          &r6);
    ok &= tlv320dac3100_read_register(0, REG_NDAC,           &r11);
    ok &= tlv320dac3100_read_register(0, REG_MDAC,           &r12);
    ok &= tlv320dac3100_read_register(0, REG_DOSR_LSB,       &r14);
    ok &= tlv320dac3100_read_register(0, REG_IFACE_CTRL1,    &r27);
    ok &= tlv320dac3100_read_register(0, REG_DAC_FLAG_1,     &r37);
    ok &= tlv320dac3100_read_register(0, REG_HEADSET_DETECT, &r67);
    ok &= tlv320dac3100_read_register(0, REG_PROC_BLOCK,     &r3c);
    ok &= tlv320dac3100_read_register(1, REG_HP_DRIVER_CTRL, &p1r31);
    ok &= tlv320dac3100_read_register(1, REG_SPK_DRIVER_CTRL, &p1r32);
    ok &= tlv320dac3100_read_register(1, REG_SPK_GAIN,       &p1r42);

    if (!ok) {
        printf("  I2C read failed — is the chip initialized?\n");
        return;
    }

    // ---- Clock tree ----
    static const char* codec_src[] = { "MCLK(!)", "BCLK", "GPIO1", "PLL_CLK" };
    static const char* pll_src[]   = { "MCLK(!)", "BCLK", "GPIO1", "DIN" };

    printf("\nClock tree:\n");
    printf("  PLL_CLKIN   = %s\n", pll_src[(r4 >> 2) & 0x03]);
    printf("  CODEC_CLKIN = %s\n", codec_src[r4 & 0x03]);

    bool pll_up    = (r5 & 0x80) != 0;
    uint8_t p_raw  = (r5 >> 4) & 0x07;
    uint8_t pll_p  = (p_raw == 0) ? 8 : p_raw;
    uint8_t r_raw  = r5 & 0x0F;
    uint8_t pll_r  = (r_raw == 0) ? 16 : r_raw;
    uint8_t pll_j  = r6 & 0x3F;

    printf("  PLL         = %s  P=%u R=%u J=%u D=0\n",
           pll_up ? "UP" : "DOWN(!)", pll_p, pll_r, pll_j);

    bool ndac_up = (r11 & 0x80) != 0;
    uint8_t ndac = r11 & 0x7F;
    if (ndac == 0) ndac = 128;

    bool mdac_up = (r12 & 0x80) != 0;
    uint8_t mdac = r12 & 0x7F;
    if (mdac == 0) mdac = 128;

    printf("  NDAC        = %u (%s)\n", ndac, ndac_up ? "up" : "down(!)");
    printf("  MDAC        = %u (%s)\n", mdac, mdac_up ? "up" : "down(!)");
    printf("  DOSR        = %u\n", r14);

    if (pll_up && ndac_up && mdac_up && r14 > 0) {
        uint32_t div = (uint32_t)ndac * mdac * r14;
        printf("  NDAC×MDAC×DOSR = %lu  (PLL_CLK / %lu = DAC_fS)\n",
               (unsigned long)div, (unsigned long)div);
    }

    // ---- Serial interface ----
    static const char* iface_mode[] = { "I2S", "DSP", "RJF", "LJF" };
    static const char* word_len[]   = { "16-bit(!)", "20-bit", "24-bit", "32-bit" };

    printf("\nSerial interface:\n");
    printf("  Mode        = %s\n", iface_mode[(r27 >> 6) & 0x03]);
    printf("  Word length = %s\n", word_len[(r27 >> 4) & 0x03]);
    printf("  BCLK        = %s\n", (r27 & 0x08) ? "output (master)" : "input (slave)");
    printf("  WCLK        = %s\n", (r27 & 0x04) ? "output (master)" : "input (slave)");
    printf("  Proc block  = PRB_P%u\n", r3c & 0x1F);

    // ---- Output drivers ----
    static const char* cm_volt[] = { "1.35V", "1.50V", "1.65V", "1.80V" };
    static const char* spk_gain_str[] = { "6dB", "12dB", "18dB", "24dB" };

    printf("\nOutput drivers (from DAC flag register + page 1):\n");
    printf("  L-DAC       = %s\n", (r37 & 0x80) ? "UP" : "down");
    printf("  R-DAC       = %s\n", (r37 & 0x08) ? "UP" : "down");
    printf("  HPL         = %s\n", (r37 & 0x20) ? "UP" : "down");
    printf("  HPR         = %s\n", (r37 & 0x02) ? "UP" : "down");
    printf("  Class-D SPK = %s\n", (p1r32 & 0x80) ? "UP" : "down");
    printf("  HP CM volt  = %s\n", cm_volt[(p1r31 >> 3) & 0x03]);
    printf("  SPK gain    = %s  muted=%s\n",
           spk_gain_str[(p1r42 >> 3) & 0x03],
           (p1r42 & 0x04) ? "no" : "YES");

    // ---- Headset detection ----
    static const char* hs_type[] = {
        "none detected", "no mic (stereo HP)", "reserved", "headset with mic"
    };
    printf("\nHeadset detection:\n");
    printf("  Enabled     = %s\n", (r67 & 0x80) ? "yes" : "no(!)");
    printf("  Detected    = %s\n", hs_type[(r67 >> 5) & 0x03]);

    printf("\n(!) = unexpected value — check init sequence\n");
}


// ======================================================
// DEBUG — STATUS FLAGS
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