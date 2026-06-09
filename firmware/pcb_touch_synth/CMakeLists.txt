# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ======================================================
# FatFS SD SPI library — carlk3/no-OS-FatFS-SD-SPI-RPi-Pico
#
# Provides: FatFS core (ff.h) + SPI SD card driver (sd_card.h)
# Pinned tag ensures reproducible builds.
# Requires: board/hw_config.c — describes the SPI bus, pins, and
#           card detect configuration for this specific board.
#
# Long Filename (LFN) support is enabled in the library's ffconf.h
# so audio files can use descriptive names (e.g. acoustic_piano.wav).
#
# FAT32 (SDHC ≤32 GB) is the recommended card format for broadest
# OS compatibility. Windows, macOS, and Linux all read/write FAT32
# natively — users load files directly in their file explorer.
# ======================================================
include(FetchContent)
FetchContent_Declare(
    no-OS-FatFS-SD-SPI-RPi-Pico
    GIT_REPOSITORY https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico.git
    GIT_TAG        master # TODO: replace with a pinned release tag after first successful
                         # configure. Check https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico/tags
    # GIT_SHALLOW TRUE intentionally omitted — shallow + branch name is unreliable
    # across CMake versions and causes "invalid reference" errors.
)
FetchContent_MakeAvailable(no-OS-FatFS-SD-SPI-RPi-Pico)

# FatFs_SPI uses relative include paths in its CMakeLists.txt which do not
# propagate correctly when the library is consumed via FetchContent.
# Retrieve the actual source directory and store it in FATFS_SPI_SOURCE_DIR
# so we can add the include paths explicitly to our target below.
FetchContent_GetProperties(no-OS-FatFS-SD-SPI-RPi-Pico
    SOURCE_DIR FATFS_SPI_SOURCE_DIR
)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Override flash size for the fitted W25Q128JV chip (128Mbit = 16MB).
# The stock 'pico' board definition assumes 2MB; this corrects the linker
# script FLASH LENGTH and sets PICO_FLASH_SIZE_BYTES for SDK consumers.
set(PICO_FLASH_SIZE_BYTES 16777216)

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(pcb_touch_synth C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# ======================================================
# FLASH-EMBEDDED VOICE CONFIG ASSET
#
# Converts assets/voice_config.txt into a C string compiled into the
# firmware binary. Used as Tier 2 fallback when no SD card is present.
#
# The generated files (voice_config_asset.c / .h) are written to the
# CMake build directory and added to the build automatically.
# They rebuild whenever assets/voice_config.txt changes.
#
# To update the embedded config: edit assets/voice_config.txt and
# recompile. Flash the new .uf2 via the BOOTSEL USB mass-storage mechanism.
# Only OSCILLATOR voices in the embedded config produce sound (PLAYBACK and
# WAVETABLE require SD card sample files that are not present in flash-only mode).
# ======================================================
set(VOICE_CONFIG_ASSET_SRC "${CMAKE_CURRENT_LIST_DIR}/assets/voice_config.txt")
set(VOICE_CONFIG_ASSET_C   "${CMAKE_CURRENT_BINARY_DIR}/voice_config_asset.c")
set(VOICE_CONFIG_ASSET_H   "${CMAKE_CURRENT_BINARY_DIR}/voice_config_asset.h")

add_custom_command(
    OUTPUT  "${VOICE_CONFIG_ASSET_C}" "${VOICE_CONFIG_ASSET_H}"
    COMMAND ${CMAKE_COMMAND}
            -DINPUT=${VOICE_CONFIG_ASSET_SRC}
            -DOUTPUT_C=${VOICE_CONFIG_ASSET_C}
            -DOUTPUT_H=${VOICE_CONFIG_ASSET_H}
            -DVAR_NAME=voice_config_asset
            -P ${CMAKE_CURRENT_LIST_DIR}/cmake/embed_text_asset.cmake
    DEPENDS "${VOICE_CONFIG_ASSET_SRC}"
            "${CMAKE_CURRENT_LIST_DIR}/cmake/embed_text_asset.cmake"
    COMMENT "Embedding assets/voice_config.txt into firmware"
    # No VERBATIM here: VERBATIM + quoted -D values causes literal quotes to appear
    # inside the variable, making the path unreadable. CMake handles the space
    # escaping correctly for -D arguments without VERBATIM on Windows.
)

# Add executable. Default name is the project name, version 0.1
add_executable(pcb_touch_synth
    pcb_touch_synth.cpp
    audio/audio_commands.cpp
    audio/audio_core.cpp
    audio/audio_manager.cpp
    audio/midi_manager.cpp
    audio/sysex_transfer.cpp
    audio/i2s_manager.cpp
    audio/ics43432.cpp
    audio/music_theory.cpp
    audio/oscillator_engine.cpp
    audio/sample_loader.cpp
    audio/synth_engine.cpp
    audio/tlv320dac3100.cpp
    audio/voice_config_parser.cpp
    audio/voice_definitions.cpp
    audio/voice_manager.cpp
    board/board_init.cpp
    board/board_power.cpp
    board/board_usb.cpp
    board/hw_config.c
    "${VOICE_CONFIG_ASSET_C}"
    controls/control_manager.cpp
    debug/debug_console.cpp
    debug/uart_console.cpp
    input/rotary_encoder.cpp
    led/dotstar.cpp
    led/led_manager.cpp
    led/led_map.cpp
    led/pca9685.cpp
    power/mp2724.cpp
    power/power_manager.cpp
    rtc/pcf85063a.cpp
    system/status_led.cpp
    system/system_init.cpp
    system/system_interrupts.cpp
    system/system_tasks.cpp
    touch/mtch2120.cpp
    touch/touch_manager.cpp
    touch/touch_map.cpp
)

pico_set_program_name(pcb_touch_synth "pcb_touch_synth")
pico_set_program_version(pcb_touch_synth "0.1")

# Generate PIO header from I2S TX program.
# This produces audio/i2s_tx.pio.h in the build directory, containing the PIO
# program binary and the i2s_tx_program_init() helper from the % c-sdk {} block.
pico_generate_pio_header(pcb_touch_synth ${CMAKE_CURRENT_LIST_DIR}/audio/i2s_tx.pio)

# stdio routing:
#   UART (GPIO 12/13) — pico_stdio_uart is linked (flag=1) so the header and
#     library are available.  uart_console_init() immediately disables the
#     driver and deinits the UART hardware so no signal is driven on the 9 cm
#     traces until uart_console_enable() is called explicitly.
#   USB  — disabled here; TinyUSB is configured as a MIDI device.
pico_enable_stdio_uart(pcb_touch_synth 1)
pico_enable_stdio_usb(pcb_touch_synth 0)

# Override default UART0 pins from the Pico board definition (GPIO 0/1)
# to match the debug header on this PCB (GPIO 12 TX, GPIO 13 RX).
# pico_enable_stdio_uart uses these when auto-initialising at startup.
target_compile_definitions(pcb_touch_synth PRIVATE
    PICO_DEFAULT_UART_TX_PIN=12
    PICO_DEFAULT_UART_RX_PIN=13
)

# Add the standard library to the build
target_link_libraries(pcb_touch_synth
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(pcb_touch_synth PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/usb                            # tusb_config.h (TinyUSB MIDI)
        ${CMAKE_CURRENT_BINARY_DIR}                              # for generated voice_config_asset.h
        ${FATFS_SPI_SOURCE_DIR}/FatFs_SPI/ff15/source            # ff.h, ffconf.h, diskio.h
        ${FATFS_SPI_SOURCE_DIR}/FatFs_SPI/sd_driver              # hw_config.h, sd_card.h, spi.h
        ${FATFS_SPI_SOURCE_DIR}/FatFs_SPI/include                # ff_stdio.h, f_util.h
)

# FatFs_SPI is an INTERFACE library — it has no compiled archive to link.
# Adding it via target_link_libraries generates a spurious -lFatFs_SPI linker
# flag that fails. Instead, add its source files directly to the executable
# and link only the hardware dependencies it requires.
target_sources(pcb_touch_synth PRIVATE
    ${FATFS_SPI_SOURCE_DIR}/FatFs_SPI/ff15/source/ff.c
    ${FATFS_SPI_SOURCE_DIR}/FatFs_SPI/ff15/source/ffunicode.c
    ${FATFS_SPI_SOURCE_DIR}/FatFs_SPI/ff15/source/ffsystem.c
    ${FATFS_SPI_SOURCE_DIR}/FatFs_SPI/sd_driver/sd_spi.c
    ${FATFS_SPI_SOURCE_DIR}/FatFs_SPI/sd_driver/spi.c
    ${FATFS_SPI_SOURCE_DIR}/FatFs_SPI/sd_driver/sd_card.c
    ${FATFS_SPI_SOURCE_DIR}/FatFs_SPI/sd_driver/crc.c
    ${FATFS_SPI_SOURCE_DIR}/FatFs_SPI/sd_driver/demo_logging.c
    ${FATFS_SPI_SOURCE_DIR}/FatFs_SPI/src/glue.c
    ${FATFS_SPI_SOURCE_DIR}/FatFs_SPI/src/f_util.c
    ${FATFS_SPI_SOURCE_DIR}/FatFs_SPI/src/ff_stdio.c
    ${FATFS_SPI_SOURCE_DIR}/FatFs_SPI/src/my_debug.c
    # rtc.c omitted — it requires hardware/rtc.h which was removed in pico-sdk 2.x.
    # get_fattime() is provided in board/hw_config.c instead.
    # sd_driver/hw_config.c is intentionally omitted — board/hw_config.c
    # provides the hardware configuration functions for this board.
    usb/usb_descriptors.c    # USB MIDI device descriptors (TinyUSB callbacks)
)

# Add any user requested libraries
target_link_libraries(pcb_touch_synth
        tinyusb_device          # TinyUSB USB device stack (MIDI class)
        tinyusb_board           # TinyUSB board-level init for RP2040
        pico_unique_id          # RP2040 unique board ID (USB serial number)
        pico_multicore           # multicore_launch_core1, inter-core FIFO
        hardware_spi
        hardware_i2c
        hardware_dma
        hardware_pio
        hardware_interp
        hardware_timer
        hardware_watchdog
        hardware_clocks
        )

pico_add_extra_outputs(pcb_touch_synth)

