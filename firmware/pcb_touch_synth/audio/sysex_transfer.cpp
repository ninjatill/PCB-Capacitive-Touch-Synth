#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/sync.h"
#include "tusb.h"
#include "ff.h"

#include "sysex_transfer.h"
#include "sysex_protocol.h"
#include "../led/dotstar.h"

// ======================================================
// RING BUFFER (inter-core: core 1 writes, core 0 reads)
// ======================================================

struct SysexRingBuffer {
    uint8_t  data[SYSEX_RING_SLOTS][SYSEX_MAX_MSG_LEN];
    uint8_t  msg_len[SYSEX_RING_SLOTS];
    volatile uint8_t read_idx;
    volatile uint8_t write_idx;
    spin_lock_t* lock;
};

static SysexRingBuffer _ring;

static void ring_init()
{
    _ring.read_idx  = 0;
    _ring.write_idx = 0;
    // Claim a hardware spin lock for the ring buffer.
    // RP2040 has 32 spinlocks; claim_unused ensures no conflict.
    _ring.lock = spin_lock_instance(spin_lock_claim_unused(true));
}

static bool ring_push(const uint8_t* msg, uint8_t len)
{
    uint32_t irq = spin_lock_blocking(_ring.lock);
    uint8_t next = (_ring.write_idx + 1u) % SYSEX_RING_SLOTS;
    if (next == _ring.read_idx) {
        spin_unlock(_ring.lock, irq);
        return false;  // full
    }
    memcpy(_ring.data[_ring.write_idx], msg, len);
    _ring.msg_len[_ring.write_idx] = len;
    _ring.write_idx = next;
    spin_unlock(_ring.lock, irq);
    return true;
}

static bool ring_pop(uint8_t* msg, uint8_t* len)
{
    uint32_t irq = spin_lock_blocking(_ring.lock);
    if (_ring.read_idx == _ring.write_idx) {
        spin_unlock(_ring.lock, irq);
        return false;  // empty
    }
    *len = _ring.msg_len[_ring.read_idx];
    memcpy(msg, _ring.data[_ring.read_idx], *len);
    _ring.read_idx = (_ring.read_idx + 1u) % SYSEX_RING_SLOTS;
    spin_unlock(_ring.lock, irq);
    return true;
}

// ======================================================
// TRANSFER STATE (written by core 0, read by core 1)
// ======================================================

static struct {
    volatile bool    active;        // core 1 polls this to know when to start
    volatile bool    core1_done;    // core 1 sets this when all blocks are queued
    volatile bool    spi_busy;      // true while core 1 has the SD file open
                                    // core 0 must not touch SPI0 (DotStar) during this
    char             filepath[SYSEX_FILEPATH_MAX];
    uint32_t         file_size;
    uint16_t         block_count;
    volatile uint32_t crc32;        // set by core 1 after all blocks sent
} _state;

bool sysex_core1_spi_busy()
{
    return _state.spi_busy;
}

// ======================================================
// CRC-8 / SMBUS  (polynomial 0x07, init 0x00)
// Used per-block to detect corruption in the ring buffer or USB stream.
// ======================================================

static uint8_t crc8_update(uint8_t crc, const uint8_t* data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc & 0x80u) ? (uint8_t)((crc << 1) ^ 0x07u) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

// ======================================================
// CRC-32 (standard IEEE 802.3 polynomial 0xEDB88320)
// Computed over the entire file; sent in TRANSFER_DONE for host verification.
// ======================================================

static const uint32_t CRC32_TABLE[16] = {
    0x00000000u, 0x1DB71064u, 0x3B6E20C8u, 0x26D930ACu,
    0x76DC4190u, 0x6B6B51F4u, 0x4DB26158u, 0x5005713Cu,
    0xEDB88320u, 0xF00F9344u, 0xD6D6A3E8u, 0xCB61B38Cu,
    0x9B64C2B0u, 0x86D3D2D4u, 0xA00AE278u, 0xBDBDF21Cu
};

static uint32_t crc32_update(uint32_t crc, const uint8_t* data, size_t len)
{
    crc ^= 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++) {
        crc = (crc >> 4) ^ CRC32_TABLE[(crc ^  data[i]      ) & 0x0Fu];
        crc = (crc >> 4) ^ CRC32_TABLE[(crc ^ (data[i] >> 4)) & 0x0Fu];
    }
    return crc ^ 0xFFFFFFFFu;
}

// ======================================================
// 7-BIT ENCODE / DECODE
// See sysex_protocol.h for the algorithm description.
// ======================================================

size_t sysex_encode_7bit(const uint8_t* src, size_t src_len, uint8_t* dst)
{
    size_t out = 0;
    for (size_t i = 0; i < src_len; i += 7u) {
        size_t n = (src_len - i < 7u) ? (src_len - i) : 7u;
        uint8_t msb = 0;
        for (size_t j = 0; j < n; j++) {
            msb |= (uint8_t)(((src[i + j] >> 7) & 1u) << j);
        }
        dst[out++] = msb;
        for (size_t j = 0; j < n; j++) {
            dst[out++] = src[i + j] & 0x7Fu;
        }
    }
    return out;
}

size_t sysex_decode_7bit(const uint8_t* src, size_t src_len, uint8_t* dst)
{
    size_t out = 0;
    for (size_t i = 0; i < src_len; i += 8u) {
        uint8_t msb = src[i];
        size_t n = (src_len - i - 1u < 7u) ? (src_len - i - 1u) : 7u;
        for (size_t j = 0; j < n; j++) {
            dst[out++] = src[i + 1u + j] | (uint8_t)(((msb >> j) & 1u) << 7);
        }
    }
    return out;
}

// ======================================================
// SYSEX MESSAGE BUILDERS
// ======================================================

static void build_header(uint8_t type, uint8_t* buf, uint8_t* len)
{
    buf[0] = 0xF0u;
    buf[1] = SYSEX_MANUFACTURER_ID;
    buf[2] = SYSEX_DEVICE_ID;
    buf[3] = type;
    *len   = 4u;
}

// Send F0 7D 00 03 [file_size 5-byte 7-bit] [block_count 2-byte raw] F7
static void push_file_header(uint32_t file_size, uint16_t block_count)
{
    uint8_t buf[SYSEX_MAX_MSG_LEN];
    uint8_t len;
    build_header(SYSEX_MSG_FILE_HEADER, buf, &len);

    // Encode file_size as 4 bytes big-endian, then 7-bit encode → 5 SysEx bytes
    uint8_t raw_size[4] = {
        (uint8_t)(file_size >> 24),
        (uint8_t)(file_size >> 16),
        (uint8_t)(file_size >>  8),
        (uint8_t)(file_size)
    };
    len += (uint8_t)sysex_encode_7bit(raw_size, 4, buf + len);

    // block_count as two raw 7-bit bytes (max 16383 blocks, ~1MB file)
    buf[len++] = (uint8_t)((block_count >> 7) & 0x7Fu);
    buf[len++] = (uint8_t)( block_count       & 0x7Fu);

    buf[len++] = 0xF7u;
    ring_push(buf, len);
}

// Build and push one DATA_BLOCK SysEx message to the ring buffer.
// block_data: SYSEX_SOURCE_BLOCK_BYTES (or fewer for the last block)
static bool push_data_block(uint16_t seq, const uint8_t* block_data, size_t block_len)
{
    uint8_t buf[SYSEX_MAX_MSG_LEN];
    uint8_t len;
    build_header(SYSEX_MSG_DATA_BLOCK, buf, &len);

    // Sequence number: two raw 7-bit bytes
    buf[len++] = (uint8_t)((seq >> 7) & 0x7Fu);
    buf[len++] = (uint8_t)( seq       & 0x7Fu);

    // 7-bit encoded block data
    len += (uint8_t)sysex_encode_7bit(block_data, block_len, buf + len);

    // CRC-8 of the unencoded block (allows host to detect per-block errors)
    buf[len++] = crc8_update(0, block_data, block_len);

    buf[len++] = 0xF7u;
    return ring_push(buf, len);
}

// Push TRANSFER_DONE with CRC32 of the entire file
static void push_transfer_done(uint32_t crc32)
{
    uint8_t buf[SYSEX_MAX_MSG_LEN];
    uint8_t len;
    build_header(SYSEX_MSG_TRANSFER_DONE, buf, &len);

    uint8_t raw_crc[4] = {
        (uint8_t)(crc32 >> 24),
        (uint8_t)(crc32 >> 16),
        (uint8_t)(crc32 >>  8),
        (uint8_t)(crc32)
    };
    len += (uint8_t)sysex_encode_7bit(raw_crc, 4, buf + len);
    buf[len++] = 0xF7u;
    ring_push(buf, len);
}

static void push_abort()
{
    uint8_t buf[8];
    uint8_t len;
    build_header(SYSEX_MSG_ABORT, buf, &len);
    buf[len++] = 0xF7u;
    ring_push(buf, len);
}

// ======================================================
// PUBLIC API — LIFECYCLE
// ======================================================

void sysex_transfer_init()
{
    ring_init();
    memset(&_state, 0, sizeof(_state));
    // spi_busy starts false — DotStar may use SPI0 freely at boot
}

bool sysex_transfer_start(const char* filepath)
{
    if (_state.active) {
        printf("SysEx: transfer already active — aborting old transfer.\n");
        sysex_transfer_abort();
    }

    // Open file and get size
    FIL fil;
    FRESULT fr = f_open(&fil, filepath, FA_READ);
    if (fr != FR_OK) {
        printf("SysEx: cannot open '%s' (FatFS error %d)\n", filepath, fr);
        return false;
    }
    _state.file_size = f_size(&fil);
    f_close(&fil);

    if (_state.file_size == 0) {
        printf("SysEx: file is empty.\n");
        return false;
    }

    uint32_t full_blocks = _state.file_size / SYSEX_SOURCE_BLOCK_BYTES;
    uint32_t remainder   = _state.file_size % SYSEX_SOURCE_BLOCK_BYTES;
    _state.block_count   = (uint16_t)(full_blocks + (remainder ? 1u : 0u));

    strncpy((char*)_state.filepath, filepath, SYSEX_FILEPATH_MAX - 1);

    printf("SysEx: starting transfer '%s'  size=%lu  blocks=%u\n",
           filepath, (unsigned long)_state.file_size, _state.block_count);

    // Send FILE_HEADER immediately from core 0 before signalling core 1
    push_file_header(_state.file_size, _state.block_count);

    // Signal core 1 to start producing DATA_BLOCK messages.
    // core1_done must be clear before setting active.
    _state.core1_done = false;
    _state.crc32      = 0;
    _state.active     = true;   // ← core 1 starts polling from here

    return true;
}

void sysex_transfer_abort()
{
    _state.spi_busy = false;
    dotstar_block_spi(false);  // software unblock
    dotstar_spi_release();     // hardware: reconnect DotStar to SPI0 bus
    _state.active   = false;
    push_abort();
    printf("SysEx: transfer aborted.\n");
}

bool sysex_transfer_is_active()
{
    return _state.active;
}

// ======================================================
// CORE 0 ROLE: drain ring buffer → TinyUSB MIDI FIFO
// ======================================================

void sysex_transfer_drain()
{
    if (!_state.active && _ring.read_idx == _ring.write_idx) return;

    uint8_t msg[SYSEX_MAX_MSG_LEN];
    uint8_t len;

    // Drain up to a few messages per loop — keeps latency low for other tasks.
    // TinyUSB TX FIFO is 128 bytes; stay under that per call.
    for (int i = 0; i < 2; i++) {
        if (!ring_pop(msg, &len)) break;
        if (tud_midi_mounted()) {
            tud_midi_stream_write(0, msg, len);
        }
    }

    // Transfer finished when core 1 is done AND ring is empty
    if (_state.active && _state.core1_done
        && _ring.read_idx == _ring.write_idx) {
        printf("SysEx: transfer complete (CRC32 0x%08lX).\n",
               (unsigned long)_state.crc32);
        _state.active = false;
    }
}

// ======================================================
// CORE 1 ROLE: read SD card, produce DATA_BLOCK messages
// ======================================================

void sysex_transfer_task()
{
    if (!_state.active || _state.core1_done) return;

    static FIL    _fil;
    static bool   _file_open  = false;
    static uint16_t _seq      = 0;
    static uint32_t _crc_acc  = 0;

    // Open file on first call after transfer starts.
    // Set spi_busy BEFORE f_open() so core 0 stops DotStar updates
    // before the first SPI transaction hits the bus.
    if (!_file_open) {
        // Physically disconnect DotStar from SPI0 before any SD card transaction.
        // dotstar_spi_acquire() drives PIN_DOTSTAR_ENABLE HIGH, tri-stating the
        // 74AHCT2G125 buffer outputs.  Without this, SD card clock and data would
        // reach the DotStar data line and corrupt the LED state (APA102 has no CS).
        dotstar_spi_acquire();     // hardware: disconnect DotStar from SPI0 bus
        _state.spi_busy = true;
        dotstar_block_spi(true);   // software: stop dotstar_show() from trying to write
        FRESULT fr = f_open(&_fil, _state.filepath, FA_READ);
        if (fr != FR_OK) {
            printf("SysEx core1: cannot open file (error %d) — aborting.\n", fr);
            _state.spi_busy = false;
            _state.active   = false;
            push_abort();
            return;
        }
        _file_open = true;
        _seq       = 0;
        _crc_acc   = 0;
        printf("SysEx core1: file opened, sending %u blocks.\n", _state.block_count);
    }

    // Read one block from SD card
    uint8_t block[SYSEX_SOURCE_BLOCK_BYTES];
    UINT    bytes_read = 0;
    FRESULT fr = f_read(&_fil, block, SYSEX_SOURCE_BLOCK_BYTES, &bytes_read);

    if (fr != FR_OK || bytes_read == 0) {
        if (fr != FR_OK) {
            printf("SysEx core1: read error %d at block %u — aborting.\n", fr, _seq);
            f_close(&_fil);
            _file_open      = false;
            _state.spi_busy = false;
            dotstar_block_spi(false);  // software unblock
            dotstar_spi_release();     // hardware: reconnect DotStar to SPI0
            _state.active   = false;
            push_abort();
            return;
        }
        // Normal end of file — release bus before signalling done
        f_close(&_fil);
        _file_open      = false;
        _state.spi_busy = false;
        dotstar_block_spi(false);  // software unblock
        dotstar_spi_release();     // hardware: reconnect DotStar to SPI0
        _state.crc32    = _crc_acc;
        push_transfer_done(_crc_acc);
        _state.core1_done = true;
        printf("SysEx core1: all blocks sent (CRC32 0x%08lX).\n",
               (unsigned long)_crc_acc);
        return;
    }

    // Accumulate CRC32 over the raw file data
    _crc_acc = crc32_update(_crc_acc, block, bytes_read);

    // Push the block — retry next call if ring buffer is full.
    // Seek back so we re-read the same chunk on the retry.
    if (!push_data_block(_seq, block, bytes_read)) {
        f_lseek(&_fil, f_tell(&_fil) - bytes_read);
        return;  // ring full; core 0 will drain, core 1 retries next iteration
    }

    _seq++;
}
