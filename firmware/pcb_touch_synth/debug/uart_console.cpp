#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/stdio_uart.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"

#include "uart_console.h"
#include "../config/board_config.h"

// ======================================================
// NOTE ON INIT STRATEGY
//
// pico_enable_stdio_uart=1 auto-initialises UART0 on GPIO 12/13 (overridden
// in CMakeLists via PICO_DEFAULT_UART_TX_PIN/RX_PIN) before main() runs.
// uart_console_init() immediately undoes that:
//   1. Disables the stdio_uart driver (stops printf routing to UART)
//   2. Deinits the UART peripheral (no clock, no power)
//   3. Returns GPIO 12/13 to high-impedance inputs (no signal on 9 cm traces)
//
// uart_console_enable() re-inits the hardware and re-enables the driver on
// demand.  The driver was already registered at startup; we just toggle it.
// ======================================================

static constexpr uint32_t UART_BAUD = 115200;
static uart_inst_t* const UART_INST = uart0;  // GPIO 12/13 are UART0

static bool _active = false;

void uart_console_init()
{
    // Undo the auto-initialisation that pico_enable_stdio_uart performed.
    // Stop printf() routing to UART immediately.
    stdio_set_driver_enabled(&stdio_uart, false);

    // Release the UART peripheral — no clock, no power draw.
    uart_deinit(UART_INST);

    // Return GPIO 12/13 to high-impedance inputs with pull-ups.
    // UART idle state is HIGH, so pull-ups prevent the trace from floating.
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_SIO);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_SIO);
    gpio_init(UART_TX_PIN);
    gpio_init(UART_RX_PIN);
    gpio_pull_up(UART_TX_PIN);
    gpio_pull_up(UART_RX_PIN);

    _active = false;
}

void uart_console_enable()
{
    if (_active) return;

    // Reclaim UART0 hardware at our baud rate.
    uart_init(UART_INST, UART_BAUD);

    // Assign UART function — TX goes idle-HIGH, 9 cm trace now driven.
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // Re-enable the stdio_uart driver.  The driver is already registered
    // (by the auto-init before main); we just unmute it.
    stdio_set_driver_enabled(&stdio_uart, true);

    _active = true;

    printf("UART console enabled: GPIO %d TX / GPIO %d RX at %u baud.\n",
           UART_TX_PIN, UART_RX_PIN, UART_BAUD);
}

void uart_console_disable()
{
    if (!_active) return;

    printf("UART console disabling — GPIO %d/%d returning to inputs.\n",
           UART_TX_PIN, UART_RX_PIN);

    stdio_set_driver_enabled(&stdio_uart, false);
    uart_deinit(UART_INST);

    gpio_set_function(UART_TX_PIN, GPIO_FUNC_SIO);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_SIO);
    gpio_init(UART_TX_PIN);
    gpio_init(UART_RX_PIN);
    gpio_pull_up(UART_TX_PIN);
    gpio_pull_up(UART_RX_PIN);

    _active = false;
}

bool uart_console_is_active()
{
    return _active;
}
