#include "main.h"

/**=================================================== *
 * ============  Hotkey Handler Routines  ============ *
 * =================================================== */

void output_toggle_hotkey_handler(device_state_t* state) {
    state->active_output ^= 1;
    switch_output(state->active_output);
};

void fw_upgrade_hotkey_handler(device_state_t* state) {
    send_value(ENABLE, FIRMWARE_UPGRADE_MSG);
    reset_usb_boot(1 << PICO_DEFAULT_LED_PIN, 0);
};

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
        hid_keyboard_report_t* report = (hid_keyboard_report_t*)packet->data;

        tud_hid_keyboard_report(REPORT_ID_KEYBOARD, report->modifier, report->keycode);
        state->last_activity[ACTIVE_OUTPUT_B] = time_us_64();
    }
}

void handle_mouse_abs_uart_msg(uart_packet_t* packet, device_state_t* state) {
    if (state->active_output == ACTIVE_OUTPUT_A) {
        const hid_abs_mouse_report_t* mouse_report = (hid_abs_mouse_report_t*)packet->data;

        tud_hid_abs_mouse_report(REPORT_ID_MOUSE, mouse_report->buttons, mouse_report->x,
                                 mouse_report->y, mouse_report->wheel, 0);

        state->last_activity[ACTIVE_OUTPUT_A] = time_us_64();
    }
}

void handle_output_select_msg(uart_packet_t* packet, device_state_t* state) {
    state->active_output = packet->data[0];
    update_leds(state);
}

void handle_fw_upgrade_msg(void) {
    reset_usb_boot(1 << PICO_DEFAULT_LED_PIN, 0);
}

void handle_mouse_zoom_msg(uart_packet_t* packet, device_state_t* state) {
    state->mouse_zoom = packet->data[0];
}

void handle_set_report_msg(uart_packet_t* packet, device_state_t* state) {
    // Only board B sends LED state through this message type
    state->keyboard_leds[ACTIVE_OUTPUT_B] = packet->data[0];
    update_leds(state);
}

// Update output variable, set LED on/off and notify the other board
void switch_output(uint8_t new_output) {
    global_state.active_output = new_output;
    update_leds(&global_state);
    send_value(new_output, OUTPUT_SELECT_MSG);
}
