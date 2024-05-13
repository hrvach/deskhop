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
 * Hotkeys to trigger actions via the keyboard.
 * ==================================================== */

hotkey_combo_t hotkeys[] = {
    /* Main keyboard switching hotkey */
    {.modifier       = 0,
     .keys           = {HOTKEY_TOGGLE},
     .key_count      = 1,
     .pass_to_os     = false,
     .action_handler = &output_toggle_hotkey_handler},

    /* Pressing right ALT + right CTRL toggles the slow mouse mode */
    {.modifier       = KEYBOARD_MODIFIER_RIGHTALT | KEYBOARD_MODIFIER_RIGHTCTRL,
     .keys           = {},
     .key_count      = 0,
     .pass_to_os     = true,
     .acknowledge    = true,
     .action_handler = &mouse_zoom_hotkey_handler},

    /* Switch lock */
    {.modifier       = KEYBOARD_MODIFIER_RIGHTCTRL,
     .keys           = {HID_KEY_L},
     .key_count      = 1,
     .acknowledge    = true,
     .action_handler = &switchlock_hotkey_handler},

    /* Screen lock */
    {.modifier       = KEYBOARD_MODIFIER_RIGHTALT | KEYBOARD_MODIFIER_RIGHTSHIFT,
     .keys           = {HID_KEY_L},
     .key_count      = 1,
     .acknowledge    = true,
     .action_handler = &screenlock_hotkey_handler},

    /* Erase stored config */
    {.modifier       = KEYBOARD_MODIFIER_RIGHTSHIFT,
     .keys           = {HID_KEY_BACKSPACE},
     .key_count      = 1,
     .pass_to_os     = true,
     .action_handler = &output_config_hotkey_handler},

    /* Erase stored config */
    {.modifier       = KEYBOARD_MODIFIER_RIGHTSHIFT,
     .keys           = {HID_KEY_F12, HID_KEY_D},
     .key_count      = 2,
     .acknowledge    = true,
     .action_handler = &wipe_config_hotkey_handler},

    /* Toggle screensaver function */
    {.modifier       = KEYBOARD_MODIFIER_RIGHTSHIFT,
     .keys           = {HID_KEY_F12, HID_KEY_S},
     .key_count      = 2,
     .acknowledge    = true,
     .action_handler = &screensaver_hotkey_handler},

    /* Record switch y coordinate  */
    {.modifier       = KEYBOARD_MODIFIER_RIGHTSHIFT,
     .keys           = {HID_KEY_F12, HID_KEY_Y},
     .key_count      = 2,
     .acknowledge    = true,
     .action_handler = &screen_border_hotkey_handler},

    /* Hold down left shift + right shift + F12 + A ==> firmware upgrade mode for board A (kbd) */
    {.modifier       = KEYBOARD_MODIFIER_RIGHTSHIFT | KEYBOARD_MODIFIER_LEFTSHIFT,
     .keys           = {HID_KEY_F12, HID_KEY_A},
     .key_count      = 2,
     .acknowledge    = true,
     .action_handler = &fw_upgrade_hotkey_handler_A},

    /* Hold down left shift + right shift + F12 + B ==> firmware upgrade mode for board B (mouse) */
    {.modifier       = KEYBOARD_MODIFIER_RIGHTSHIFT | KEYBOARD_MODIFIER_LEFTSHIFT,
     .keys           = {HID_KEY_F12, HID_KEY_B},
     .key_count      = 2,
     .acknowledge    = true,
     .action_handler = &fw_upgrade_hotkey_handler_B}};

/* ============================================================ *
 * Detect if any hotkeys were pressed
 * ============================================================ */

/* Tries to find if the keyboard report contains key, returns true/false */
bool key_in_report(uint8_t key, const hid_keyboard_report_t *report) {
    for (int j = 0; j < KEYS_IN_USB_REPORT; j++) {
        if (key == report->keycode[j]) {
            return true;
        }
    }

    return false;
}

/* Check if the current report matches a specific hotkey passed on */
bool check_specific_hotkey(hotkey_combo_t keypress, const hid_keyboard_report_t *report) {
    /* We expect all modifiers specified to be detected in the report */
    if (keypress.modifier != (report->modifier & keypress.modifier))
        return false;

    for (int n = 0; n < keypress.key_count; n++) {
        if (!key_in_report(keypress.keys[n], report)) {
            return false;
        }
    }

    /* Getting here means all of the keys were found. */
    return true;
}

/* Go through the list of hotkeys, check if any of them match. */
hotkey_combo_t *check_all_hotkeys(hid_keyboard_report_t *report, device_t *state) {
    for (int n = 0; n < ARRAY_SIZE(hotkeys); n++) {
        if (check_specific_hotkey(hotkeys[n], report)) {
            return &hotkeys[n];
        }
    }

    return NULL;
}

/* ==================================================== *
 * Keyboard Queue Section
 * ==================================================== */

void process_kbd_queue_task(device_t *state) {
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

void queue_kbd_report(hid_keyboard_report_t *report, device_t *state) {
    /* It wouldn't be fun to queue up a bunch of messages and then dump them all on host */
    if (!state->tud_connected)
        return;

    queue_try_add(&state->kbd_queue, report);
}

void release_all_keys(device_t *state) {
    static hid_keyboard_report_t no_keys_pressed_report = {0, 0, {0}};
    queue_try_add(&state->kbd_queue, &no_keys_pressed_report);
}

/* If keys need to go locally, queue packet to kbd queue, else send them through UART */
void send_key(hid_keyboard_report_t *report, device_t *state) {
    if (CURRENT_BOARD_IS_ACTIVE_OUTPUT) {
        queue_kbd_report(report, state);
        state->last_activity[BOARD_ROLE] = time_us_64();
    } else {
        send_packet((uint8_t *)report, KEYBOARD_REPORT_MSG, KBD_REPORT_LENGTH);
    }
}

/* Decide if consumer control reports go local or to the other board */
void send_consumer_control(uint8_t *raw_report, device_t *state) {
    if (CURRENT_BOARD_IS_ACTIVE_OUTPUT) {
        tud_hid_n_report(0, REPORT_ID_CONSUMER, raw_report, CONSUMER_CONTROL_LENGTH);
        state->last_activity[BOARD_ROLE] = time_us_64();
    } else {
        send_packet((uint8_t *)raw_report, CONSUMER_CONTROL_MSG, CONSUMER_CONTROL_LENGTH);
    }
}

/* ==================================================== *
 * Parse and interpret the keys pressed on the keyboard
 * ==================================================== */

void process_keyboard_report(uint8_t *raw_report, int length, device_t *state) {
    hid_keyboard_report_t *keyboard_report = (hid_keyboard_report_t *)raw_report;
    hotkey_combo_t *hotkey                 = NULL;

    if (length < KBD_REPORT_LENGTH)
        return;

    /* If the report is longer by 1 byte, we can assume the first byte is the report ID
       and that the keyboard didn't switch to boot mode properly. Use this workaround
       until full HID report parsing is implemented */
    if (length == KBD_REPORT_LENGTH + 1)
        keyboard_report = (hid_keyboard_report_t *)(raw_report + 1);

    /* Check if any hotkey was pressed */
    hotkey = check_all_hotkeys(keyboard_report, state);

    /* ... and take appropriate action */
    if (hotkey != NULL) {
        /* Provide visual feedback we received the action */
        if (hotkey->acknowledge)
            blink_led(state);

        /* Execute the corresponding handler */
        hotkey->action_handler(state, keyboard_report);

        /* And pass the key to the output PC if configured to do so. */
        if (!hotkey->pass_to_os)
            return;
    }

    /* This method will decide if the key gets queued locally or sent through UART */
    send_key(keyboard_report, state);
}

void process_consumer_report(uint8_t *raw_report, int length, device_t *state) {
    uint8_t new_report[CONSUMER_CONTROL_LENGTH] = {0};

    /* We expect length not to be zero or bail out */
    if (!length)
        return;

    /* Consumer control report ID rewrite and forward */
    if (raw_report[0] && raw_report[0] == global_state.kbd_dev.consumer_report_id) {
        for (int i = 0; i < length - 1 || i < CONSUMER_CONTROL_LENGTH; i++) {
            new_report[i] = raw_report[i + 1];
        }

        send_consumer_control(new_report, &global_state);
    }
}