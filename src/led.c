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

/**==================================================== *
 * ========== Update pico and keyboard LEDs  ========== *
 * ==================================================== */

void update_leds(device_state_t* state) {
    gpio_put(GPIO_LED_PIN, state->active_output == BOARD_ROLE);

    // TODO: Will be done in a callback
    if (BOARD_ROLE == PICO_A) {
        uint8_t* leds = &(state->keyboard_leds[state->active_output]);
        tuh_hid_set_report(global_state.kbd_dev_addr, global_state.kbd_instance, 0,
                           HID_REPORT_TYPE_OUTPUT, leds, sizeof(uint8_t));
    }
}
