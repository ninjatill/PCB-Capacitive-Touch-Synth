# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(pcb_touch_synth C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
add_executable(pcb_touch_synth
    pcb_touch_synth.cpp
    audio/audio_commands.cpp
    audio/audio_core.cpp
    audio/audio_manager.cpp
    audio/i2s_manager.cpp
    audio/synth_engine.cpp
    audio/tlv320dac3100.cpp
    audio/voice_manager.cpp
    board/board_init.cpp
    board/board_power.cpp
    board/board_usb.cpp
    controls/control_manager.cpp
    input/rotary_encoder.cpp
    led/dotstar.cpp
    led/led_manager.cpp
    led/led_map.cpp
    led/pca9685.cpp
    power/mp2724.cpp
    power/power_manager.cpp
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

# Generate PIO header
#pico_generate_pio_header(pcb_touch_synth ${CMAKE_CURRENT_LIST_DIR}/blink.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(pcb_touch_synth 0)
pico_enable_stdio_usb(pcb_touch_synth 1)

# Add the standard library to the build
target_link_libraries(pcb_touch_synth
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(pcb_touch_synth PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(pcb_touch_synth 
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

