#pragma once

// ======================================================
// BOARD PIN CONFIGURATION
//
// Central GPIO pin map for the PCB Piano RP2040 board.
// All peripheral drivers reference these defines — change
// a pin here and it propagates to the entire firmware.
//
// Hardware overview:
//   MCU:     RP2040 (dual-core Cortex-M0+, 133MHz)
//   Flash:   128Mbit QSPI external flash
//   I2C0:    Touch controllers    (PIN_I2C0_SDA / PIN_I2C0_SCL)
//   I2C1:    Audio, Power, LEDs   (PIN_I2C1_SDA / PIN_I2C1_SCL)
//   SPI0:    SD card + DotStar LEDs share bus
//            (PIN_SPI0_SCK / PIN_SPI0_TX / PIN_SPI0_RX / PIN_SPI0_CS_SD)
//            DotStar uses PIN_DOTSTAR_CLK / PIN_DOTSTAR_DATA (same physical pins as SPI0 SCK/TX)
//   SD det:  PIN_SD_CARD_DETECT — active-LOW, 10kΩ pull-up on PCB
//   I2S out: DAC audio output  (PIN_AUDIO_I2S_BCLK / PIN_AUDIO_I2S_LRCLK / PIN_AUDIO_I2S_DATA)
//   I2S in:  MEMS mic input    (PIN_MIC_I2S_DATA, shares BCLK/LRCLK with the DAC output)
//   UART:    Debug header       (UART_TX_PIN / UART_RX_PIN, not yet enabled in firmware)
//
// GPIO ALLOCATION SUMMARY (RP2040 GPIO 0-29)
//   0  SPI0_RX       1  SPI0_CS_SD    2  SPI0_SCK/DOTSTAR_CLK  3  SPI0_TX/DOTSTAR_DATA
//   4  I2C0_SDA      5  I2C0_SCL      6  I2C1_SDA              7  I2C1_SCL
//   8  SD_CARD_DETECT  9  CHARGER_IRQ  10  USB_MUX_SEL         11  DEBUG_GPIO11
//  12  UART_TX       13 UART_RX       14  (unassigned)          15  TOUCH1_IRQ
//  16  TOUCH2_IRQ   17  TOUCH_RESET  18  DOTSTAR_ENABLE         19  5V_EN
//  20  LED_EN        21 AUDIO_VOL_A  22  AUDIO_VOL_B            23  MIC_I2S_DATA
//  24  DEBUG_GPIO24  25 AUDIO_RESET  26  AUDIO_I2S_BCLK         27  AUDIO_I2S_LRCLK
//  28  AUDIO_I2S_DATA  29 AUDIO_IRQ
// ======================================================

// ======================================================
// I2C BUSES
// ======================================================

// I2C0
#define PIN_I2C0_SDA    4
#define PIN_I2C0_SCL    5

// I2C1
#define PIN_I2C1_SDA    6
#define PIN_I2C1_SCL    7

// ======================================================
// SPI BUS
// ======================================================

#define PIN_SPI0_SCK    2
#define PIN_SPI0_TX     3
#define PIN_SPI0_RX     0
#define PIN_SPI0_CS_SD  1

// ======================================================
// DOTSTAR STATUS LEDS
// ======================================================

#define PIN_DOTSTAR_CLK     2
#define PIN_DOTSTAR_DATA    3
#define PIN_DOTSTAR_ENABLE  18

// ======================================================
// LED CONTROLLERS
// ======================================================

#define PIN_LED_EN      20

// ======================================================
// TOUCH CONTROLLER
// ======================================================

#define PIN_TOUCH1_IRQ    15
#define PIN_TOUCH2_IRQ    16
#define PIN_TOUCH_RESET   17

// ======================================================
// AUDIO
// ======================================================

#define PIN_AUDIO_I2S_BCLK      26
#define PIN_AUDIO_I2S_LRCLK     27
#define PIN_AUDIO_I2S_DATA      28
#define PIN_AUDIO_RESET         25
#define PIN_AUDIO_IRQ           29

#define PIN_AUDIO_VOL_A         21
#define PIN_AUDIO_VOL_B         22

// ======================================================
// MICROPHONE I2S INPUT (ICS-43432)
//
// PIN_MIC_I2S_DATA is the mic's SD output (data back to RP2040).
// The mic shares PIN_AUDIO_I2S_BCLK and PIN_AUDIO_I2S_LRCLK with
// the TLV320DAC3100 DAC — both devices see the same BCLK and WCLK
// from the RP2040 I2S master at AUDIO_BCLK_HZ (44100 × 64 = 2,822,400 Hz).
//
// Hardwired on PCB (no MCU GPIO needed):
//   CONFIG pin → GND   (required by ICS-43432 datasheet)
//   LR pin     → 3V3   (mic outputs on RIGHT channel; data follows WS rising edge)
// The SD line has a 100kΩ pull-down to GND (confirmed on PCB).
// ======================================================

#define PIN_MIC_I2S_DATA        23

// ======================================================
// BATTERY CHARGER / POWER PATH
// ======================================================

#define PIN_CHARGER_IRQ          9
#define PIN_USB_MUX_SEL         10
#define PIN_5V_EN               19

// ======================================================
// SD CARD
//
// PIN_SD_DETECT: mechanical card-detect switch in the SD slot.
// Active-LOW: card inserted = LOW, card absent = HIGH.
// Circuit: GPIO → 10kΩ pull-up to 3.3V (on PCB) → switch → GND.
// Configure as GPIO_IN with internal pull-up DISABLED (pull-up is external).
// Can be used for edge-triggered interrupt on card insert/remove.
// ======================================================

#define PIN_SD_CARD_DETECT   8

// ======================================================
// FUTURE USE / HARDWARE ALLOCATIONS ON PCB (not yet implemented in firmware)
// ======================================================

#define UART_TX_PIN         12
#define UART_RX_PIN         13
#define DEBUG_GPIO11        11
#define DEBUG_GPIO24        24




