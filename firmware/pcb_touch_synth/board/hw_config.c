/*
 * hw_config.c  —  FatFS SD SPI hardware descriptor (C, not C++)
 *
 * Implements the four functions declared in hw_config.h that the
 * carlk3/no-OS-FatFS-SD-SPI-RPi-Pico library calls at runtime to discover
 * the SPI bus and SD card configuration for this board.
 *
 * Struct member names are taken verbatim from the library's spi.h and
 * sd_card.h headers. Do not rename or reorder fields.
 *
 * SPI BUS SHARING (DotStar LEDs):
 *   SPI0 is shared between the SD card and the DotStar LEDs via the
 *   74AHCT2G125 level shifter (/OE = PIN_DOTSTAR_ENABLE).
 *   All SD card operations in sample_loader.cpp are bracketed with
 *   dotstar_spi_acquire() / dotstar_spi_release() to prevent conflicts.
 *   This file only describes the hardware — bus arbitration is the
 *   caller's responsibility.
 */

#include "hw_config.h"
#include "../config/board_config.h"

/* -----------------------------------------------------------------------
 * SPI bus configuration
 * Member names from FatFs_SPI/sd_driver/spi.h
 * ----------------------------------------------------------------------- */
static spi_t spis[] = {
    {
        .hw_inst  = spi0,
        .miso_gpio = PIN_SPI0_RX,    /* GPIO 0 */
        .mosi_gpio = PIN_SPI0_TX,    /* GPIO 3 */
        .sck_gpio  = PIN_SPI0_SCK,   /* GPIO 2 */
        .baud_rate = 12500000,       /* 12.5 MHz data transfer rate */
        .DMA_IRQ_num = DMA_IRQ_0,    /* DMA_IRQ_1 reserved for future I2S DMA */
        /* All other members (tx_dma, rx_dma, initialized, etc.) are
           state variables managed by the library — leave at zero init. */
    }
};

/* -----------------------------------------------------------------------
 * SD card configuration
 * Member names from FatFs_SPI/sd_driver/sd_card.h
 * ----------------------------------------------------------------------- */
static sd_card_t sd_cards[] = {
    {
        .pcName           = "0:",              /* FatFS drive label */
        .spi              = &spis[0],
        .ss_gpio          = PIN_SPI0_CS_SD,    /* GPIO 1 — chip select */
        .use_card_detect  = true,
        .card_detect_gpio = PIN_SD_CARD_DETECT,/* GPIO 8 */
        .card_detected_true = 0,               /* active-low: LOW = card present */
        /* State variables (m_Status, sectors, card_type, mounted, etc.)
           are managed by the library — leave at zero init. */
    }
};

/* -----------------------------------------------------------------------
 * Library API — exact signatures declared in hw_config.h.
 * Do not rename these functions.
 * ----------------------------------------------------------------------- */

size_t sd_get_num()
{
    return sizeof(sd_cards) / sizeof(sd_cards[0]);
}

sd_card_t *sd_get_by_num(size_t num)
{
    if (num < sd_get_num()) {
        return &sd_cards[num];
    }
    return NULL;
}

size_t spi_get_num()
{
    return sizeof(spis) / sizeof(spis[0]);
}

spi_t *spi_get_by_num(size_t num)
{
    if (num < spi_get_num()) {
        return &spis[num];
    }
    return NULL;
}

/* -----------------------------------------------------------------------
 * FatFS timestamp callback
 *
 * get_fattime() is now implemented in rtc/pcf85063a.cpp (C++ with
 * extern "C" linkage) so it can return real RTC time from the PCF85063A.
 * When the RTC is not initialised or the OS flag is set, it falls back
 * to a fixed 2026-01-01 epoch.  No stub is needed here.
 * ----------------------------------------------------------------------- */
