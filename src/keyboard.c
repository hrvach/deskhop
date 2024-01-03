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

/* ==================================================== *
 * Hotkeys to trigger actions via the keyboard
 * ==================================================== */

hotkey_combo_t hotkeys[] = {
    /* Main keyboard switching hotkey */
    {.modifier = 0,
     .keys = {HOTKEY_TOGGLE},
     .key_count = 1,
     .action_handler = &output_toggle_hotkey_handler},

    /* Holding down right ALT slows the mouse down */
    {.modifier = KEYBOARD_MODIFIER_RIGHTALT, 
     .keys = {}, 
     .key_count = 0,
     .action_handler = &mouse_zoom_hotkey_handler},

    /* Switch lock */
    {.modifier = KEYBOARD_MODIFIER_RIGHTCTRL,
     .keys = {HID_KEY_L},
     .key_count = 1,
     .action_handler = &switchlock_hotkey_handler},

    /* Hold down left shift + right shift + F12 + A ==> firmware upgrade mode for board A (kbd) */
    {.modifier = KEYBOARD_MODIFIER_RIGHTSHIFT | KEYBOARD_MODIFIER_LEFTSHIFT,
     .keys = {HID_KEY_F12, HID_KEY_A},
     .key_count = 2,
     .action_handler = &fw_upgrade_hotkey_handler_A},

    /* Hold down left shift + right shift + F12 + B ==> firmware upgrade mode for board B (mouse) */
    {.modifier = KEYBOARD_MODIFIER_RIGHTSHIFT | KEYBOARD_MODIFIER_LEFTSHIFT,
     .keys = {HID_KEY_F12, HID_KEY_B},
     .key_count = 2,
     .action_handler = &fw_upgrade_hotkey_handler_B}
};


/* ==================================================== *
 * Keyboard Queue Section
 * ==================================================== */

void process_kbd_queue_task(device_state_t *state) {
    hid_keyboard_report_t report;

    /* If we're not connected, we have nowhere to send reports to. */
    if (!state->tud_connected)
        return;

    /* Peek first, if there is anything there... */
    if (!queue_try_peek(&state->kbd_queue, &report))
        return;

    /* ... try sending it to the host, if it's successful */
    bool succeeded = tud_hid_keyboard_report(REPORT_ID_KEYBOARD, report.modifier, report.keycode);

    /* ... then we can remove it from the queue. Race conditions shouldn't happen [tm] */
    if (succeeded)
        queue_try_remove(&state->kbd_queue, &report);
}

void queue_kbd_report(hid_keyboard_report_t *report, device_state_t *state) {
    /* It wouldn't be fun to queue up a bunch of messages and then dump them all on host */
    if (!state->tud_connected)
        return;
        
    queue_try_add(&state->kbd_queue, report);
}

void stop_pressing_any_keys(device_state_t *state) {    
    static hid_keyboard_report_t no_keys_pressed_report = {0, 0, {0}};
    queue_try_add(&state->kbd_queue, &no_keys_pressed_report);
}

/* ==================================================== *
 * Parse and interpret the keys pressed on the keyboard 
 * ==================================================== */

bool no_keys_are_pressed(const hid_keyboard_report_t* report) {
    if (report->modifier != 0)
        return false;

    for (int n = 0; n < KEYS_IN_USB_REPORT; n++) {
        if (report->keycode[n] != 0)
            return false;
    }
    return true;
}

void process_keyboard_report(uint8_t* raw_report, int length, device_state_t* state) {
    hid_keyboard_report_t* keyboard_report = (hid_keyboard_report_t*)raw_report;

    if (length < KBD_REPORT_LENGTH)
        return;

    /* Go through the list of hotkeys, check if any are pressed, then execute their handler */
    for (int n = 0; n < sizeof(hotkeys) / sizeof(hotkeys[0]); n++) {
        if (keypress_check(hotkeys[n], keyboard_report)) {
            hotkeys[n].action_handler(state);
            return;
        }
    }

    state->key_pressed = !no_keys_are_pressed(keyboard_report);

    /* If no keys are pressed anymore, take care of checking and deactivating stuff */
    if (!state->key_pressed) {
        all_keys_released_handler(state);
    } 

    /* If keys need to go to output B, send them through UART, otherwise send a HID report directly */
    if (state->active_output == ACTIVE_OUTPUT_B) {
        send_packet(raw_report, KEYBOARD_REPORT_MSG, KBD_REPORT_LENGTH);
    } else {        
        queue_kbd_report(keyboard_report, state);
        state->last_activity[ACTIVE_OUTPUT_A] = time_us_64();
    }
}


/* ============================================================ *
 * Check if a specific key combination is present in the report 
 * ============================================================ */

bool keypress_check(hotkey_combo_t keypress, const hid_keyboard_report_t* report) {
    int matches = 0;

    /* We expect all modifiers specified to be detected in the report */
    if (keypress.modifier != (report->modifier & keypress.modifier))
        return false;

    for (int n = 0; n < keypress.key_count; n++) {
        for (int j = 0; j < KEYS_IN_USB_REPORT; j++) {
            if (keypress.keys[n] == report->keycode[j]) {
                matches++;
                break;
            }
        }
        /* If any of the keys are not found, we can bail out early. */
        if (matches < n + 1) {
            return false;
        }
    }

    /* Getting here means all of the keys were found. */
    return true;
}