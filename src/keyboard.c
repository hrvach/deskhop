#include "main.h"

/* ==================================================== *
 * Hotkeys to trigger actions via the keyboard
 * ==================================================== */

hotkey_combo_t hotkeys[] = {
    // Main keyboard switching hotkey
    {.modifier = 0,
     .keys = {HOTKEY_TOGGLE},
     .key_count = 1,
     .action_handler = &output_toggle_hotkey_handler},

    // Holding down right ALT slows the mouse down
    {.modifier = KEYBOARD_MODIFIER_RIGHTALT, 
     .keys = {}, 
    .key_count = 0,
    .action_handler = &mouse_zoom_hotkey_handler},

    // Hold down left shift + right shift + P + H + X ==> firmware upgrade mode
    {.modifier = KEYBOARD_MODIFIER_RIGHTSHIFT | KEYBOARD_MODIFIER_LEFTSHIFT,
     .keys = {HID_KEY_P, HID_KEY_H, HID_KEY_X},
     .key_count = 3,
     .action_handler = &fw_upgrade_hotkey_handler}
};


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

    // Go through the list of hotkeys, check if any are pressed, then execute their handler
    for (int n = 0; n < sizeof(hotkeys) / sizeof(hotkeys[0]); n++) {
        if (keypress_check(hotkeys[n], keyboard_report)) {
            hotkeys[n].action_handler(state);
            return;
        }
    }   

    // If no keys are pressed anymore, take care of checking and deactivating stuff
    if (no_keys_are_pressed(keyboard_report)) {
        all_keys_released_handler(state);
    } 

    // If keys need to go to output B, send them through UART, otherwise send a HID report directly
    if (state->active_output == ACTIVE_OUTPUT_B) {
        send_packet(raw_report, KEYBOARD_REPORT_MSG, KBD_REPORT_LENGTH);
    } else {
        tud_hid_keyboard_report(REPORT_ID_KEYBOARD, keyboard_report->modifier,
                                keyboard_report->keycode);

        state->last_activity[ACTIVE_OUTPUT_A] = time_us_64();
    }
}


/* ============================================================ *
 * Check if a specific key combination is present in the report 
 * ============================================================ */

bool keypress_check(hotkey_combo_t keypress, const hid_keyboard_report_t* report) {
    int matches = 0;

    // We expect all modifiers specified to be detected in the report
    if (keypress.modifier != (report->modifier & keypress.modifier))
        return false;

    for (int n = 0; n < keypress.key_count; n++) {
        for (int j = 0; j < KEYS_IN_USB_REPORT; j++) {
            if (keypress.keys[n] == report->keycode[j]) {
                matches++;
                break;
            }
        }
        // If any of the keys are not found, we can bail out early.
        if (matches < n + 1) {
            return false;
        }
    }

    // Getting here means all of the keys were found.
    return true;
}

