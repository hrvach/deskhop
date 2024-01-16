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
     .pass_to_os = true,
     .action_handler = &mouse_zoom_hotkey_handler},

    /* Switch lock */
    {.modifier = KEYBOARD_MODIFIER_RIGHTCTRL,
     .keys = {HID_KEY_L},
     .key_count = 1,
     .acknowledge = true,
     .action_handler = &switchlock_hotkey_handler},

    /* Erase stored config */
    {.modifier = KEYBOARD_MODIFIER_RIGHTSHIFT,
     .keys = {HID_KEY_F12, HID_KEY_D},
     .key_count = 2,
     .acknowledge = true,
     .action_handler = &wipe_config_hotkey_handler},

    /* Record switch y coordinate  */
    {.modifier = KEYBOARD_MODIFIER_RIGHTSHIFT,
     .keys = {HID_KEY_F12, HID_KEY_Y},
     .key_count = 2,
     .acknowledge = true,
     .action_handler = &screen_border_hotkey_handler},

    /* Hold down left shift + right shift + F12 + A ==> firmware upgrade mode for board A (kbd) */
    {.modifier = KEYBOARD_MODIFIER_RIGHTSHIFT | KEYBOARD_MODIFIER_LEFTSHIFT,
     .keys = {HID_KEY_F12, HID_KEY_A},
     .key_count = 2,
     .acknowledge = true,
     .action_handler = &fw_upgrade_hotkey_handler_A},

    /* Hold down left shift + right shift + F12 + B ==> firmware upgrade mode for board B (mouse) */
    {.modifier = KEYBOARD_MODIFIER_RIGHTSHIFT | KEYBOARD_MODIFIER_LEFTSHIFT,
     .keys = {HID_KEY_F12, HID_KEY_B},
     .key_count = 2,
     .acknowledge = true,
     .action_handler = &fw_upgrade_hotkey_handler_B}};

/* ==================================================== *
 * Keyboard Queue Section
 * ==================================================== */

void process_kbd_queue_task(device_state_t* state) {
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

void queue_kbd_report(hid_keyboard_report_t* report, device_state_t* state) {
    /* It wouldn't be fun to queue up a bunch of messages and then dump them all on host */
    if (!state->tud_connected)
        return;

    queue_try_add(&state->kbd_queue, report);
}

void stop_pressing_any_keys(device_state_t* state) {
    static hid_keyboard_report_t no_keys_pressed_report = {0, 0, {0}};
    queue_try_add(&state->kbd_queue, &no_keys_pressed_report);
}

/* If keys need to go locally, queue packet to kbd queue, else send them through UART */
void send_key(hid_keyboard_report_t* report, device_state_t* state) {
    if (state->active_output == BOARD_ROLE) {
        queue_kbd_report(report, state);
        state->last_activity[BOARD_ROLE] = time_us_64();
    } else {
        send_packet((uint8_t*)report, KEYBOARD_REPORT_MSG, KBD_REPORT_LENGTH);
    }
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

hotkey_combo_t* check_hotkeys(hid_keyboard_report_t* report, int length, device_state_t* state) {    
    /* Go through the list of hotkeys, check if any are pressed, then execute their handler */    
    for (int n = 0; n < sizeof(hotkeys) / sizeof(hotkeys[0]); n++) {
        if (is_key_pressed(hotkeys[n], report)) {
            return &hotkeys[n];
        }
    }

    return NULL;
}

void process_keyboard_report(uint8_t* raw_report, int length, device_state_t* state) {
    hid_keyboard_report_t* keyboard_report = (hid_keyboard_report_t*)raw_report;
    hotkey_combo_t* hotkey = NULL;

    if (length < KBD_REPORT_LENGTH)
        return;
   
    /* If no keys are pressed anymore, take care of checking and deactivating stuff */
    if(no_keys_are_pressed(keyboard_report))
        all_keys_released_handler(state);
    else
        /* Check if it was a hotkey, makes sense only if a key is pressed */
        hotkey = check_hotkeys(keyboard_report, length, state);

    /* ... and take appropriate action */
    if(hotkey != NULL) {
        /* Provide visual feedback we received the action */
        if (hotkey->acknowledge)
            blink_led(state);
        
        hotkey->action_handler(state);
        if (!hotkey->pass_to_os)
            return;
    }
    
    /* This method will decide if the key gets queued locally or sent through UART */
    send_key(keyboard_report, state);
}

/* ============================================================ *
 * Check if a specific key combination is present in the report
 * ============================================================ */

bool is_key_pressed(hotkey_combo_t keypress, const hid_keyboard_report_t* report) {
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