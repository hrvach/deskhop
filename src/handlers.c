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

/* This is the main hotkey for switching outputs */
void output_toggle_hotkey_handler(device_t *state, hid_keyboard_report_t *report) {
    /* If switching explicitly disabled, return immediately */
    if (state->switch_lock)
        return;

    state->active_output ^= 1;
    switch_output(state, state->active_output);
};

void _get_border_position(device_t *state, border_size_t *border) {
    /* To avoid having 2 different keys, if we're above half, it's the top coord */
    if (state->pointer_y > (MAX_SCREEN_COORD / 2))
        border->bottom = state->pointer_y;
    else
        border->top = state->pointer_y;
}

void _screensaver_set(device_t *state, uint8_t value) {
    if (CURRENT_BOARD_IS_ACTIVE_OUTPUT)
        state->config.output[BOARD_ROLE].screensaver.mode = value;
    else
        send_value(value, SCREENSAVER_MSG);
};

/* This key combo records switch y top coordinate for different-size monitors  */
void screen_border_hotkey_handler(device_t *state, hid_keyboard_report_t *report) {
    border_size_t *border = &state->config.output[state->active_output].border;
    if (CURRENT_BOARD_IS_ACTIVE_OUTPUT) {
        _get_border_position(state, border);
        save_config(state);
    }

    queue_packet((uint8_t *)border, SYNC_BORDERS_MSG, sizeof(border_size_t));
};

/* This key combo puts board A in firmware upgrade mode */
void fw_upgrade_hotkey_handler_A(device_t *state, hid_keyboard_report_t *report) {
    reset_usb_boot(1 << PICO_DEFAULT_LED_PIN, 0);
};

/* This key combo puts board B in firmware upgrade mode */
void fw_upgrade_hotkey_handler_B(device_t *state, hid_keyboard_report_t *report) {
    send_value(ENABLE, FIRMWARE_UPGRADE_MSG);
};

/* This key combo prevents mouse from switching outputs */
void switchlock_hotkey_handler(device_t *state, hid_keyboard_report_t *report) {
    state->switch_lock ^= 1;
    send_value(state->switch_lock, SWITCH_LOCK_MSG);
}

/* This key combo toggles gaming mode */
void toggle_gaming_mode_handler(device_t *state, hid_keyboard_report_t *report) {
    state->gaming_mode ^= 1;
    send_value(state->gaming_mode, GAMING_MODE_MSG);
};

/* This key combo locks both outputs simultaneously */
void screenlock_hotkey_handler(device_t *state, hid_keyboard_report_t *report) {
    hid_keyboard_report_t lock_report = {0}, release_keys = {0};

    for (int out = 0; out < NUM_SCREENS; out++) {
        switch (state->config.output[out].os) {
            case WINDOWS:
            case LINUX:
                lock_report.modifier   = KEYBOARD_MODIFIER_LEFTGUI;
                lock_report.keycode[0] = HID_KEY_L;
                break;
            case MACOS:
                lock_report.modifier   = KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_LEFTGUI;
                lock_report.keycode[0] = HID_KEY_Q;
                break;
            default:
                break;
        }

        if (BOARD_ROLE == out) {
            queue_kbd_report(&lock_report, state);
            release_all_keys(state);
        } else {
            queue_packet((uint8_t *)&lock_report, KEYBOARD_REPORT_MSG, KBD_REPORT_LENGTH);
            queue_packet((uint8_t *)&release_keys, KEYBOARD_REPORT_MSG, KBD_REPORT_LENGTH);
        }
    }
}

/* When pressed, erases stored config in flash and loads defaults on both boards */
void wipe_config_hotkey_handler(device_t *state, hid_keyboard_report_t *report) {
    wipe_config();
    load_config(state);
    send_value(ENABLE, WIPE_CONFIG_MSG);
}

/* When pressed, toggles the current mouse zoom mode state */
void mouse_zoom_hotkey_handler(device_t *state, hid_keyboard_report_t *report) {
    state->mouse_zoom ^= 1;
    send_value(state->mouse_zoom, MOUSE_ZOOM_MSG);
};

/* When pressed, enables the screensaver on active output */
void enable_screensaver_hotkey_handler(device_t *state, hid_keyboard_report_t *report) {
    uint8_t desired_mode = state->config.output[BOARD_ROLE].screensaver.mode;

    /* If the user explicitly asks for screensaver to be active, ignore config and turn it on */
    if (desired_mode == DISABLED)
        desired_mode = PONG;

    _screensaver_set(state, desired_mode);
}

/* When pressed, disables the screensaver on active output */
void disable_screensaver_hotkey_handler(device_t *state, hid_keyboard_report_t *report) {
    _screensaver_set(state, DISABLED);
}

/* Put the device into a special configuration mode */
void config_enable_hotkey_handler(device_t *state, hid_keyboard_report_t *report) {
    /* If config mode is already active, skip this and reboot to return to normal mode */
    if (!state->config_mode_active) {
        watchdog_hw->scratch[5] = MAGIC_WORD_1;
        watchdog_hw->scratch[6] = MAGIC_WORD_2;
    }

    release_all_keys(state);
    state->reboot_requested = true;
};


/**==================================================== *
 * ==========  UART Message Handling Routines  ======== *
 * ==================================================== */

/* Function handles received keypresses from the other board */
void handle_keyboard_uart_msg(uart_packet_t *packet, device_t *state) {
    queue_kbd_report((hid_keyboard_report_t *)packet->data, state);
    state->last_activity[BOARD_ROLE] = time_us_64();
}

/* Function handles received mouse moves from the other board */
void handle_mouse_abs_uart_msg(uart_packet_t *packet, device_t *state) {
    mouse_report_t *mouse_report = (mouse_report_t *)packet->data;
    queue_mouse_report(mouse_report, state);

    state->pointer_x       = mouse_report->x;
    state->pointer_y       = mouse_report->y;
    state->mouse_buttons   = mouse_report->buttons;

    state->last_activity[BOARD_ROLE] = time_us_64();
}

/* Function handles request to switch output  */
void handle_output_select_msg(uart_packet_t *packet, device_t *state) {
    state->active_output = packet->data[0];
    if (state->tud_connected)
        release_all_keys(state);

    restore_leds(state);
}

/* On firmware upgrade message, reboot into the BOOTSEL fw upgrade mode */
void handle_fw_upgrade_msg(uart_packet_t *packet, device_t *state) {
    reset_usb_boot(1 << PICO_DEFAULT_LED_PIN, 0);
}

/* Comply with request to turn mouse zoom mode on/off  */
void handle_mouse_zoom_msg(uart_packet_t *packet, device_t *state) {
    state->mouse_zoom = packet->data[0];
}

/* Process request to update keyboard LEDs */
void handle_set_report_msg(uart_packet_t *packet, device_t *state) {
    /* We got this via serial, so it's stored to the opposite of our board role */
    state->keyboard_leds[OTHER_ROLE] = packet->data[0];

    /* If we have a keyboard we can control leds on, restore state if active */
    if (global_state.keyboard_connected && !CURRENT_BOARD_IS_ACTIVE_OUTPUT)
        restore_leds(state);
}

/* Process request to block mouse from switching, update internal state */
void handle_switch_lock_msg(uart_packet_t *packet, device_t *state) {
    state->switch_lock = packet->data[0];
}

/* Handle border syncing message that lets the other device know about monitor height offset */
void handle_sync_borders_msg(uart_packet_t *packet, device_t *state) {
    border_size_t *border = &state->config.output[state->active_output].border;

    if (CURRENT_BOARD_IS_ACTIVE_OUTPUT) {
        _get_border_position(state, border);
        queue_packet((uint8_t *)border, SYNC_BORDERS_MSG, sizeof(border_size_t));
    } else
        memcpy(border, packet->data, sizeof(border_size_t));

    save_config(state);
}

/* When this message is received, flash the locally attached LED to verify serial comms */
void handle_flash_led_msg(uart_packet_t *packet, device_t *state) {
    blink_led(state);
}

/* When this message is received, wipe the local flash config */
void handle_wipe_config_msg(uart_packet_t *packet, device_t *state) {
    wipe_config();
    load_config(state);
}

/* Update screensaver state after received message */
void handle_screensaver_msg(uart_packet_t *packet, device_t *state) {
    state->config.output[BOARD_ROLE].screensaver.mode = packet->data[0];
}

/* Process consumer control message */
void handle_consumer_control_msg(uart_packet_t *packet, device_t *state) {
    queue_cc_packet(packet->data, state);
}

/* Process request to store config to flash */
void handle_save_config_msg(uart_packet_t *packet, device_t *state) {
    save_config(state);
}

/* Process request to reboot the board */
void handle_reboot_msg(uart_packet_t *packet, device_t *state) {
    reboot();
}

/* Decapsulate and send to the other box */
void handle_proxy_msg(uart_packet_t *packet, device_t *state) {
    queue_packet(&packet->data[1], (enum packet_type_e)packet->data[0], PACKET_DATA_LENGTH - 1);
}

/* Process relative mouse command */
void handle_toggle_gaming_msg(uart_packet_t *packet, device_t *state) {
    state->gaming_mode = packet->data[0];
}

/* Process api communication messages */
void handle_api_msgs(uart_packet_t *packet, device_t *state) {
    uint8_t value_idx = packet->data[0];
    const field_map_t *map = get_field_map_entry(value_idx);

    /* If we don't have a valid map entry, return immediately */
    if (map == NULL)
        return;

    /* Create a pointer to the offset into the structure we need to access */
    uint8_t *ptr = (((uint8_t *)&global_state) + map->offset);

    if (packet->type == SET_VAL_MSG) {
        /* Not allowing writes to objects defined as read-only */
        if (map->readonly)
            return;

        memcpy(ptr, &packet->data[1], map->len);
    }
    else if (packet->type == GET_VAL_MSG) {
        uart_packet_t response = {.type=GET_VAL_MSG, .data={[0] = value_idx}};
        memcpy(&response.data[1], ptr, map->len);
        queue_cfg_packet(&response, state);
    }

    /* With each GET/SET message, we reset the configuration mode timeout */
    reset_config_timer(state);
}

/* Handle the "read all" message by calling our "read one" handler for each type */
void handle_api_read_all_msg(uart_packet_t *packet, device_t *state) {
    uart_packet_t result = {.type=GET_VAL_MSG};

    for (int i = 0; i < get_field_map_length(); i++) {
        result.data[0] = get_field_map_index(i)->idx;
        handle_api_msgs(&result, state);
    }
}

/* Process request packet and create a response */
void handle_request_byte_msg(uart_packet_t *packet, device_t *state) {
    uint32_t address = packet->data32[0];

    if (address > STAGING_IMAGE_SIZE)
        return;

    /* Add requested data to bytes 4-7 in the packet and return it with a different type */
    uint32_t data = *(uint32_t *)&ADDR_FW_RUNNING[address];
    packet->data32[1] = data;

    queue_packet(packet->data, RESPONSE_BYTE_MSG, PACKET_DATA_LENGTH);
}

/* Process response message following a request we sent to read a byte */
/* state->page_offset and state->page_number are kept locally and compared to returned values */
void handle_response_byte_msg(uart_packet_t *packet, device_t *state) {
    uint16_t offset = packet->data[0];
    uint32_t address = packet->data32[0];

    if (address != state->fw.address) {
        state->fw.upgrade_in_progress = false;
        state->fw.address = 0;
        return;
    }
    else {
        /* Provide visual feedback of the ongoing copy by toggling LED for every sector */
        if((address & 0xfff) == 0x000)
            toggle_led();
    }

    /* Update checksum as we receive each byte */
    if (address < STAGING_IMAGE_SIZE - FLASH_SECTOR_SIZE)
        for (int i=0; i<4; i++)
            state->fw.checksum = crc32_iter(state->fw.checksum, packet->data[4 + i]);

    memcpy(state->page_buffer + offset, &packet->data32[1], sizeof(uint32_t));

    /* Neeeeeeext byte, please! */
    state->fw.address += sizeof(uint32_t);
    state->fw.byte_done = true;
}

/* Process a request to read a firmware package from flash */
void handle_heartbeat_msg(uart_packet_t *packet, device_t *state) {
    uint16_t other_running_version = packet->data16[0];

    if (state->fw.upgrade_in_progress)
        return;

    /* If the other board isn't running a newer version, we are done */
    if (other_running_version <= state->_running_fw.version)
        return;

    /* It is? Ok, kick off the firmware upgrade */
    state->fw = (fw_upgrade_state_t) {
        .upgrade_in_progress = true,
        .byte_done = true,
        .address = 0,
        .checksum = 0xffffffff,
    };
}


/**==================================================== *
 * ==============  Output Switch Routines  ============ *
 * ==================================================== */

/* Update output variable, set LED on/off and notify the other board so they are in sync. */
void switch_output(device_t *state, uint8_t new_output) {
    state->active_output = new_output;
    restore_leds(state);
    send_value(new_output, OUTPUT_SELECT_MSG);

    /* If we were holding a key down and drag the mouse to another screen, the key gets stuck.
       Changing outputs = no more keypresses on the previous system. */
    release_all_keys(state);
}
