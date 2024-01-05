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

#include "main.h"

/*********  Global Variable  **********/
device_state_t global_state = {0};

/**================================================== *
 * ==============  Main Program Loops  ============== *
 * ================================================== */

void main(void) {
    // Wait for the board to settle
    sleep_ms(10);

    // Initial board setup
    initial_setup();

    // Initial state, A is the default output
    switch_output(ACTIVE_OUTPUT_A);

    while (true) {
        // USB device task, needs to run as often as possible
        tud_task();

        // Verify core1 is still running and if so, reset watchdog timer
        kick_watchdog();

        // Check if there were any keypresses and send them
        process_kbd_queue_task(&global_state);

        // Check if there were any mouse movements and send them
        process_mouse_queue_task(&global_state);
    }
    
}

void core1_main() {
    uart_packet_t in_packet = {0};

    while (true) {
        // Update the timestamp, so core0 can figure out if we're dead
        global_state.core1_last_loop_pass = time_us_64();

        // USB host task, needs to run as often as possible
        if (tuh_inited())	
            tuh_task();

        // Receives data over serial from the other board
        receive_char(&in_packet, &global_state);
    }
}

/* =======  End of Main Program Loops  ======= */
