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

/**=================================================== *
 * ============  Hotkey Handler Routines  ============ *
 * =================================================== */

void output_toggle_hotkey_handler(device_state_t* state) {
    /* If switching explicitly disabled, return immediately */
    if (state->switch_lock)
        return;

    state->active_output ^= 1;
    switch_output(state->active_output);
};

/* This key combo puts board A in firmware upgrade mode */
void fw_upgrade_hotkey_handler_A(device_state_t* state) {
    // reset_usb_boot(1 << PICO_DEFAULT_LED_PIN, 0);
    // TODO : support both pico and pico_w
    reset_usb_boot(0, 0);
};

/* This key combo puts board B in firmware upgrade mode */
void fw_upgrade_hotkey_handler_B(device_state_t* state) {
    send_value(ENABLE, FIRMWARE_UPGRADE_MSG);
};


void switchlock_hotkey_handler(device_state_t* state) {
    state->switch_lock ^= 1;
    send_value(state->switch_lock, SWITCH_LOCK_MSG);    
}


void mouse_zoom_hotkey_handler(device_state_t* state) {
    if (state->mouse_zoom)
        return;

    send_value(ENABLE, MOUSE_ZOOM_MSG);
    state->mouse_zoom = true;
};

void all_keys_released_handler(device_state_t* state) {
    if (state->mouse_zoom) {
        state->mouse_zoom = false;
        send_value(DISABLE, MOUSE_ZOOM_MSG);
    }
};

/**==================================================== *
 * ==========  UART Message Handling Routines  ======== *
 * ==================================================== */

void handle_keyboard_uart_msg(uart_packet_t* packet, device_state_t* state) {
    if (state->active_output == ACTIVE_OUTPUT_B) {
        queue_kbd_report((hid_keyboard_report_t*)packet->data, state);
        state->last_activity[ACTIVE_OUTPUT_B] = time_us_64();
    }
}

void handle_mouse_abs_uart_msg(uart_packet_t* packet, device_state_t* state) {
    hid_abs_mouse_report_t* mouse_report = (hid_abs_mouse_report_t*)packet->data;        
    queue_mouse_report(mouse_report, state);                                
    state->last_activity[ACTIVE_OUTPUT_A] = time_us_64();
}

void handle_output_select_msg(uart_packet_t* packet, device_state_t* state) {
    state->active_output = packet->data[0];
    if (state->tud_connected)
        stop_pressing_any_keys(&global_state);
    
    update_leds(state);
}

/* On firmware upgrade message, reboot into the BOOTSEL fw upgrade mode */
void handle_fw_upgrade_msg(void) {
    // reset_usb_boot(1 << PICO_DEFAULT_LED_PIN, 0);
    // TODO : support both pico and pico_w
    reset_usb_boot(0, 0);
}

void handle_mouse_zoom_msg(uart_packet_t* packet, device_state_t* state) {
    state->mouse_zoom = packet->data[0];
}

void handle_set_report_msg(uart_packet_t* packet, device_state_t* state) {
    /* Only board B sends LED state through this message type */
    state->keyboard_leds[ACTIVE_OUTPUT_B] = packet->data[0];
    update_leds(state);
}

void handle_switch_lock_msg(uart_packet_t* packet, device_state_t* state) {
    state->switch_lock = packet->data[0];    
}

/* Update output variable, set LED on/off and notify the other board so they are in sync. */
void switch_output(uint8_t new_output) {
    global_state.active_output = new_output;
    update_leds(&global_state);
    send_value(new_output, OUTPUT_SELECT_MSG);

    /* If we were holding a key down and drag the mouse to another screen, the key gets stuck.
       Changing outputs = no more keypresses on the previous system. */
    stop_pressing_any_keys(&global_state);
}
