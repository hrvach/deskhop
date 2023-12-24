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

usb_device_t* pio_usb_init(void) {
    static pio_usb_configuration_t config = PIO_USB_DEFAULT_CONFIG;
    config.pin_dp = 14;
    config.alarm_pool = (void*)alarm_pool_create(2, 1);

    return pio_usb_host_init(&config);
}

/* ================================================== *
 * Perform initial board/usb setup
 * ================================================== */

usb_device_t* initial_setup(void) {
    /* PIO USB requires a clock multiple of 12 MHz, setting to 120 MHz */
    set_sys_clock_khz(120000, true);

    /* Init and enable the on-board LED GPIO as output */
    gpio_init(GPIO_LED_PIN);
    gpio_set_dir(GPIO_LED_PIN, GPIO_OUT);

    /* Initialize and configure UART */
    serial_init();

    /* Initialize and configure TinyUSB */
    tusb_init();

    /* Setup RP2040 Core 1 */
    multicore_reset_core1();
    multicore_launch_core1(core1_main);

    /* Initialize and configure PIO USB */
    usb_device_t* pio_usb_device = pio_usb_init();

    /* Update the core1 initial pass timestamp before enabling the watchdog */
    global_state.core1_last_loop_pass = time_us_64();

    /* Setup the watchdog so we reboot and recover from a crash */
    watchdog_enable(WATCHDOG_TIMEOUT, WATCHDOG_PAUSE_ON_DEBUG);

    return pio_usb_device;
}

/* ==========  End of Initial Board Setup  ========== */