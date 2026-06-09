# Third-party licenses

This firmware is released under the **GNU General Public License v3.0** (see
`LICENSE` in the repository root). It incorporates the following third-party
components, each with their own license terms. Both are compatible with GPL v3.

---

## FatFS — Generic FAT Filesystem Module

- **Author:** ChaN
- **Source:** http://elm-chan.org/fsw/ff/
- **License:** FatFS License (BSD 1-clause style) — see `fatfs/license.txt`
- **Used by:** `audio/sample_loader.cpp` (via no-OS-FatFS-SD-SPI-RPi-Pico wrapper)
- **GPL v3 compatibility:** Compatible — permissive, no copyleft conditions

---

## no-OS-FatFS-SD-SPI-RPi-Pico

- **Author:** Carl John Kugler III
- **Source:** https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico
- **License:** Apache License 2.0 — see `no-os-fatfs-sd-spi-rpi-pico/license.txt`
- **Used by:** `audio/sample_loader.cpp`, `board/hw_config.c`
- **GPL v3 compatibility:** Compatible — the FSF explicitly states Apache 2.0 is
  compatible with GPL v3. See https://www.gnu.org/licenses/license-compatibility.html

---

## Raspberry Pi Pico SDK

- **Author:** Raspberry Pi Ltd
- **Source:** https://github.com/raspberrypi/pico-sdk
- **License:** BSD 3-Clause — included with the SDK distribution
- **Used by:** All firmware files
- **GPL v3 compatibility:** Compatible — permissive BSD licence

---

*These notices satisfy the attribution requirements of the Apache 2.0 and
FatFS licenses. The combined firmware as a whole is distributed under GPL v3.*
