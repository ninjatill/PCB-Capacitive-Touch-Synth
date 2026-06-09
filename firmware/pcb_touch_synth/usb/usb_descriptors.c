/*
 * usb_descriptors.c — USB MIDI 1.0 device descriptors
 *
 * Implements the three TinyUSB descriptor callbacks:
 *   tud_descriptor_device_cb()
 *   tud_descriptor_configuration_cb()
 *   tud_descriptor_string_cb()
 *
 * USB structure:
 *   Device class 0x00 (class specified at interface level)
 *   Configuration 1
 *     Interface 0 — AudioControl (AC)
 *       Class-specific AC header
 *     Interface 1 — MIDIStreaming (MS)
 *       Class-specific MS header
 *       MIDI IN  Jack (Embedded, ID=1) — data from keyboard to host
 *       MIDI OUT Jack (Embedded, ID=2) — data from host to keyboard
 *       Bulk IN  Endpoint 0x81 + Class-specific MS endpoint
 *       Bulk OUT Endpoint 0x01 + Class-specific MS endpoint
 *
 * VID/PID: 0x2E8A / 0x9001
 *   0x2E8A = Raspberry Pi / RP2040 ecosystem VID
 *   0x9001 = PCB Touch Synth (custom PID, not assigned by RPi)
 *   Register a unique PID at pid.codes if distributing publicly.
 */

#include "tusb.h"
#include "pico/unique_id.h"

/* -----------------------------------------------------------------------
 * String indices
 * ----------------------------------------------------------------------- */
enum {
    STRID_LANGID = 0,
    STRID_MANUFACTURER,
    STRID_PRODUCT,
    STRID_SERIAL,
    STRID_MIDI_INTERFACE
};

/* -----------------------------------------------------------------------
 * Endpoint numbers
 * ----------------------------------------------------------------------- */
#define EPNUM_MIDI_OUT  0x01u   /* OUT: host → device (MIDI from host) */
#define EPNUM_MIDI_IN   0x81u   /* IN:  device → host (MIDI from keyboard) */
#define EP_SIZE         64u

/* -----------------------------------------------------------------------
 * Interface numbers
 * ----------------------------------------------------------------------- */
#define ITF_NUM_AUDIO_CONTROL   0
#define ITF_NUM_MIDI_STREAMING  1
#define ITF_NUM_TOTAL           2

/* -----------------------------------------------------------------------
 * Configuration descriptor total length
 *
 * Config header:       9
 * AC interface:        9
 * AC class header:     9
 * MS interface:        9
 * MS class header:     7
 * MIDI IN Jack:        6
 * MIDI OUT Jack:       9
 * Bulk IN endpoint:    9
 * MS Bulk IN EP:       5
 * Bulk OUT endpoint:   9
 * MS Bulk OUT EP:      5
 * Total:              86
 * ----------------------------------------------------------------------- */
#define MS_HEADER_LEN   7u
#define MS_JACKS_LEN    (6u + 9u)          /* IN jack + OUT jack */
#define MS_EP_LEN       (9u + 5u + 9u + 5u) /* IN ep + MS IN ep + OUT ep + MS OUT ep */
#define MS_TOTAL_LEN    (MS_HEADER_LEN + MS_JACKS_LEN + MS_EP_LEN)  /* 50 */

#define CONFIG_TOTAL_LEN (9u + 9u + 9u + 9u + MS_TOTAL_LEN)  /* 86 */

/* -----------------------------------------------------------------------
 * Device descriptor
 * ----------------------------------------------------------------------- */
tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200u,
    .bDeviceClass       = 0x00u,    /* class at interface level */
    .bDeviceSubClass    = 0x00u,
    .bDeviceProtocol    = 0x00u,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0x2E8Au,  /* Raspberry Pi / RP2040 */
    .idProduct          = 0x9001u,  /* PCB Touch Synth MIDI — custom PID */
    .bcdDevice          = 0x0100u,  /* device version 1.0 */
    .iManufacturer      = STRID_MANUFACTURER,
    .iProduct           = STRID_PRODUCT,
    .iSerialNumber      = STRID_SERIAL,
    .bNumConfigurations = 1u
};

uint8_t const * tud_descriptor_device_cb(void)
{
    return (uint8_t const *)&desc_device;
}

/* -----------------------------------------------------------------------
 * Configuration descriptor (with MIDI streaming)
 * ----------------------------------------------------------------------- */
static uint8_t const desc_configuration[] =
{
    /* ---- Configuration ---- */
    9,                              /* bLength */
    TUSB_DESC_CONFIGURATION,        /* bDescriptorType */
    U16_TO_U8S_LE(CONFIG_TOTAL_LEN),/* wTotalLength */
    ITF_NUM_TOTAL,                  /* bNumInterfaces */
    1,                              /* bConfigurationValue */
    0,                              /* iConfiguration */
    0x80u,                          /* bmAttributes: bus-powered */
    250,                            /* bMaxPower: 500 mA */

    /* ---- Interface 0: AudioControl ---- */
    9,                              /* bLength */
    TUSB_DESC_INTERFACE,            /* bDescriptorType */
    ITF_NUM_AUDIO_CONTROL,          /* bInterfaceNumber */
    0,                              /* bAlternateSetting */
    0,                              /* bNumEndpoints */
    TUSB_CLASS_AUDIO,               /* bInterfaceClass */
    AUDIO_SUBCLASS_CONTROL,         /* bInterfaceSubClass */
    AUDIO_FUNC_PROTOCOL_CODE_UNDEF, /* bInterfaceProtocol */
    0,                              /* iInterface */

    /* ---- Class-specific AudioControl header ---- */
    9,                              /* bLength */
    TUSB_DESC_CS_INTERFACE,         /* bDescriptorType */
    AUDIO_CS_AC_INTERFACE_HEADER,   /* bDescriptorSubType */
    U16_TO_U8S_LE(0x0100u),         /* bcdADC 1.00 */
    U16_TO_U8S_LE(9u),              /* wTotalLength (just this header) */
    1,                              /* bInCollection: 1 streaming interface */
    ITF_NUM_MIDI_STREAMING,         /* baInterfaceNr[0] */

    /* ---- Interface 1: MIDIStreaming ---- */
    9,                              /* bLength */
    TUSB_DESC_INTERFACE,            /* bDescriptorType */
    ITF_NUM_MIDI_STREAMING,         /* bInterfaceNumber */
    0,                              /* bAlternateSetting */
    2,                              /* bNumEndpoints (IN + OUT) */
    TUSB_CLASS_AUDIO,               /* bInterfaceClass */
    AUDIO_SUBCLASS_MIDI_STREAMING,  /* bInterfaceSubClass */
    AUDIO_FUNC_PROTOCOL_CODE_UNDEF, /* bInterfaceProtocol */
    STRID_MIDI_INTERFACE,           /* iInterface */

    /* ---- Class-specific MS interface header ---- */
    7,                              /* bLength */
    TUSB_DESC_CS_INTERFACE,         /* bDescriptorType */
    MIDI_CS_INTERFACE_HEADER,       /* bDescriptorSubType */
    U16_TO_U8S_LE(0x0100u),         /* bcdMSC 1.00 */
    U16_TO_U8S_LE(MS_TOTAL_LEN),    /* wTotalLength */

    /* ---- MIDI IN Jack (Embedded, ID=1) ----
     * Data path: keyboard generates → USB IN endpoint → host.
     * The host receives note-on/off events from the keyboard. */
    6,                              /* bLength */
    TUSB_DESC_CS_INTERFACE,         /* bDescriptorType */
    MIDI_CS_INTERFACE_IN_JACK,      /* bDescriptorSubType */
    MIDI_JACK_EMBEDDED,             /* bJackType */
    1,                              /* bJackID */
    0,                              /* iJack */

    /* ---- MIDI OUT Jack (Embedded, ID=2) ----
     * Data path: host sends → USB OUT endpoint → keyboard.
     * The keyboard receives note-on/off for the LED light show. */
    9,                              /* bLength */
    TUSB_DESC_CS_INTERFACE,         /* bDescriptorType */
    MIDI_CS_INTERFACE_OUT_JACK,     /* bDescriptorSubType */
    MIDI_JACK_EMBEDDED,             /* bJackType */
    2,                              /* bJackID */
    1,                              /* bNrInputPins */
    1,                              /* BaSourceID[0]: connected to IN jack 1 */
    1,                              /* BaSourcePin[0] */
    0,                              /* iJack */

    /* ---- Bulk IN Endpoint 0x81 (keyboard → host) ---- */
    9,                              /* bLength */
    TUSB_DESC_ENDPOINT,             /* bDescriptorType */
    EPNUM_MIDI_IN,                  /* bEndpointAddress (0x81 = EP1 IN) */
    TUSB_XFER_BULK,                 /* bmAttributes */
    U16_TO_U8S_LE(EP_SIZE),         /* wMaxPacketSize */
    0,                              /* bInterval (bulk: ignored) */
    0,                              /* bRefresh */
    0,                              /* bSynchAddress */

    /* ---- Class-specific MS Bulk IN endpoint ---- */
    5,                              /* bLength */
    TUSB_DESC_CS_ENDPOINT,          /* bDescriptorType */
    AUDIO_CS_EP_SUBTYPE_GENERAL,    /* bDescriptorSubType */
    1,                              /* bNumEmbMIDIJack */
    1,                              /* BaAssocJackID[0]: IN jack 1 */

    /* ---- Bulk OUT Endpoint 0x01 (host → keyboard) ---- */
    9,                              /* bLength */
    TUSB_DESC_ENDPOINT,             /* bDescriptorType */
    EPNUM_MIDI_OUT,                 /* bEndpointAddress (0x01 = EP1 OUT) */
    TUSB_XFER_BULK,                 /* bmAttributes */
    U16_TO_U8S_LE(EP_SIZE),         /* wMaxPacketSize */
    0,                              /* bInterval */
    0,                              /* bRefresh */
    0,                              /* bSynchAddress */

    /* ---- Class-specific MS Bulk OUT endpoint ---- */
    5,                              /* bLength */
    TUSB_DESC_CS_ENDPOINT,          /* bDescriptorType */
    AUDIO_CS_EP_SUBTYPE_GENERAL,    /* bDescriptorSubType */
    1,                              /* bNumEmbMIDIJack */
    2                               /* BaAssocJackID[0]: OUT jack 2 */
};

uint8_t const * tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
    return desc_configuration;
}

/* -----------------------------------------------------------------------
 * String descriptors
 * ----------------------------------------------------------------------- */
static char serial_str[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];

static uint16_t const string_langid[] = { (TUSB_DESC_STRING << 8) | 4, 0x0409 };

static const char * const string_desc[] = {
    (const char *)string_langid,  /* 0: language ID */
    "Anthropic / ninjatill",      /* 1: manufacturer */
    "PCB Touch Synth MIDI",       /* 2: product */
    serial_str,                   /* 3: serial (filled from unique board ID) */
    "MIDI Controller"             /* 4: MIDI streaming interface */
};

uint16_t const * tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void)langid;

    /* Lazily build the serial string from the RP2040 unique board ID. */
    if (serial_str[0] == '\0') {
        pico_unique_board_id_t id;
        pico_get_unique_board_id(&id);
        for (int i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES; i++) {
            static const char hex[] = "0123456789ABCDEF";
            serial_str[i * 2 + 0] = hex[(id.id[i] >> 4) & 0xFu];
            serial_str[i * 2 + 1] = hex[(id.id[i] >> 0) & 0xFu];
        }
        serial_str[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES] = '\0';
    }

    if (index >= sizeof(string_desc) / sizeof(string_desc[0])) {
        return NULL;
    }

    /* Convert ASCII string to UTF-16 USB string descriptor. */
    static uint16_t desc_str[32];
    const char * str = string_desc[index];

    /* String 0 is already the raw language-ID descriptor. */
    if (index == 0) {
        return (uint16_t const *)str;
    }

    uint8_t chr_count = 0;
    for (uint8_t i = 0; str[i] != '\0' && chr_count < 31; i++, chr_count++) {
        desc_str[1 + chr_count] = str[i];
    }

    /* Header: length (bytes) + descriptor type */
    desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2u * chr_count + 2u));

    return desc_str;
}
