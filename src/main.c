#include "main.h"

/*********  Global Variable  **********/
device_state_t global_state = {0};

/**================================================== *
 * ==============  Main Program Loops  ============== *
 * ================================================== */

void main(void) {
    // Wait for the board to settle
    sleep_ms(10);

    global_state.usb_device = initial_setup();

    // Initial state, A is the default output
    switch_output(ACTIVE_OUTPUT_A);

    while (true) {
        // USB device task, needs to run as often as possible
        tud_task();

        // If we are not yet connected to the PC, don't bother with host
        // If host task becomes too slow, move it to the second core
        if (global_state.tud_connected) {
            // Execute HOST task periodically
            pio_usb_host_task();

            // Query devices and handle reports
            if (global_state.usb_device && global_state.usb_device->connected) {
                check_endpoints(&global_state);
            }
        }

        // Verify core1 is still running and if so, reset watchdog timer
        kick_watchdog();
    }
}

void core1_main() {
    uart_packet_t in_packet = {0};

    while (true) {
        // Update the timestamp, so core0 can figure out if we're dead
        global_state.core1_last_loop_pass = time_us_64();

        receive_char(&in_packet, &global_state);
    }
}

/* =======  End of Main Program Loops  ======= */
