/*
 * This file is part of DeskHop (https://github.com/hrvach/deskhop).
 * Copyright (c) 2024 Hrvoje Cavrak
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/**================================================== *
 * =============  Initial Board Setup  ============== *
 * ================================================== */

#include "main.h"

/* ================================================== *
 * Perform initial UART setup
 * ================================================== */

void serial_init() {
    /* Set up our UART with a default baudrate. */
    uart_init(SERIAL_UART, SERIAL_BAUDRATE);

    /* Set UART flow control CTS/RTS. We don't have these - turn them off.*/
    uart_set_hw_flow(SERIAL_UART, false, false);

    /* Set our data format */
    uart_set_format(SERIAL_UART, SERIAL_DATA_BITS, SERIAL_STOP_BITS, SERIAL_PARITY);

    /* Turn of CRLF translation */
    uart_set_translate_crlf(SERIAL_UART, false);

    /* We do want FIFO, will help us have fewer interruptions */
    uart_set_fifo_enabled(SERIAL_UART, true);

    /* Set the RX/TX pins, they differ based on the device role (A or B, check
    /* schematics) */
    gpio_set_function(SERIAL_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(SERIAL_RX_PIN, GPIO_FUNC_UART);
}

/* ================================================== *
 * PIO USB configuration, D+ pin 14, D- pin 15
 * ================================================== */

void pio_usb_host_config(void) {
    /* tuh_configure() must be called before tuh_init() */
    static pio_usb_configuration_t config = PIO_USB_DEFAULT_CONFIG;
    config.pin_dp                         = PIO_USB_DP_PIN_DEFAULT;
    tuh_configure(BOARD_TUH_RHPORT, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &config);

    /* Initialize and configure TinyUSB Host */
    tuh_init(1);
}

/* ================================================== *
 * Perform initial board/usb setup
 * ================================================== */

void initial_setup(device_t *state) {
    /* PIO USB requires a clock multiple of 12 MHz, setting to 120 MHz */
    set_sys_clock_khz(120000, true);

    /* Search the persistent storage sector in flash for valid config or use defaults */
    load_config(state);

    /* Init and enable the on-board LED GPIO as output */
    gpio_init(GPIO_LED_PIN);
    gpio_set_dir(GPIO_LED_PIN, GPIO_OUT);

    /* Initialize and configure UART */
    serial_init();

    /* Initialize keyboard and mouse queues */
    queue_init(&state->kbd_queue, sizeof(hid_keyboard_report_t), KBD_QUEUE_LENGTH);
    queue_init(&state->mouse_queue, sizeof(mouse_abs_report_t), MOUSE_QUEUE_LENGTH);

    /* Setup RP2040 Core 1 */
    multicore_reset_core1();
    multicore_launch_core1(core1_main);

    /* Initialize and configure TinyUSB Device */
    tud_init(BOARD_TUD_RHPORT);

    /* Initialize and configure TinyUSB Host */
    pio_usb_host_config();

    /* Update the core1 initial pass timestamp before enabling the watchdog */
    state->core1_last_loop_pass = time_us_64();

    /* Setup the watchdog so we reboot and recover from a crash */
    watchdog_enable(WATCHDOG_TIMEOUT, WATCHDOG_PAUSE_ON_DEBUG);
}

/* ==========  End of Initial Board Setup  ========== */