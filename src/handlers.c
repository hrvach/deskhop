/*
 * This file is part of DeskHop (https://github.com/hrvach/deskhop).
 * Copyright (c) 2025 Hrvoje Cavrak
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * See the file LICENSE for the full license text.
 */

#include "main.h"

/* =================================================== *
 * ============  Hotkey Handler Routines  ============ *
 * =================================================== */

/* This is the main hotkey for switching outputs */
void output_toggle_hotkey_handler(device_t *state, hid_keyboard_report_t *report) {
    /* If switching explicitly disabled, return immediately */
    if (state->switch_lock)
        return;

    /* If we're already tracking a press, don't re-trigger */
    if (state->toggle_hotkey_press_time != 0)
        return;

    /* Record when the hotkey was first pressed */
    state->toggle_hotkey_press_time = time_us_64();

    /* Switch immediately - we'll decide on release whether to stay or switch back */
    state->active_output ^= 1;
    set_active_output(state, state->active_output);
};

void _get_border_position(device_t *state, border_size_t *border) {
    int y = state->pointer_y;

    /* Check if this is the default/uninitialized state [0, MAX] */
    if (border->start == 0 && border->end == MAX_SCREEN_COORD) {
        /* First press - set both to current Y as starting point */
        border->start = y;
        border->end = y;
    } else if (border->start == border->end) {
        /* Second press - expand range in the appropriate direction */
        if (y < border->start)
            border->start = y;
        else
            border->end = y;
    } else {
        /* Already have a range - update whichever boundary is closer */
        int dist_start = y > border->start ? y - border->start : border->start - y;
        int dist_end = y > border->end ? y - border->end : border->end - y;

        if (dist_start < dist_end)
            border->start = y;
        else
            border->end = y;

        /* Normalize: ensure start <= end */
        if (border->start > border->end) {
            int temp = border->start;
            border->start = border->end;
            border->end = temp;
        }
    }
}

void _screensaver_set(device_t *state, uint8_t value) {
    if (CURRENT_BOARD_IS_ACTIVE_OUTPUT)
        state->config.output[BOARD_ROLE].screensaver.mode = value;
    else
        send_value(value, SCREENSAVER_MSG);
};

/* Send a SET_VAL_MSG to the other device to sync a config value */
void _send_set_val_msg(uint8_t index, int32_t value) {
    uint8_t data[PACKET_DATA_LENGTH] = {0};
    data[0] = index;
    memcpy(&data[1], &value, sizeof(int32_t));
    queue_packet(data, SET_VAL_MSG, PACKET_DATA_LENGTH);
}

/* Sync the computer border to the other device after local save.
 * Both devices need both from and to ranges for coordinate mapping to work. */
void _sync_computer_border(device_t *state) {
    screen_transition_t *border = &state->config.computer_border;

    /* Send all 4 values to the other device */
    _send_set_val_msg(83, border->from.start);
    _send_set_val_msg(84, border->from.end);
    _send_set_val_msg(85, border->to.start);
    _send_set_val_msg(86, border->to.end);

    /* Tell the other device to save its config */
    queue_packet(NULL, SAVE_CONFIG_MSG, 0);
}

/* Core logic for saving screen border - runs on device with the mouse.
 * Determines which border to set based on screen_index and cursor position:
 * - On primary screen near the computer border: sets the computer-switching border
 * - Otherwise: sets the appropriate screen_transition border for multi-monitor setups
 */
void _save_screen_border(device_t *state) {
    output_t *output = &state->config.output[state->active_output];
    int idx = output->screen_index;
    bool cursor_toward_border = (output->pos == RIGHT && state->pointer_x < MAX_SCREEN_COORD / 2) ||
                                (output->pos == LEFT && state->pointer_x > MAX_SCREEN_COORD / 2);

    border_size_t *border;
    bool is_computer_border = false;

    if (idx == 1 && cursor_toward_border) {
        /* On primary screen, cursor toward computer border: set computer-switching border */
        if (state->active_output == 0)
            border = &state->config.computer_border.from;
        else
            border = &state->config.computer_border.to;
        is_computer_border = true;
    } else if (idx == 1) {
        /* On primary screen, cursor away from border: set transition 0 "from" (1→2) */
        border = &output->screen_transition[0].from;
    } else if (idx == output->screen_count) {
        /* On last screen: set the "to" border for returning */
        border = &output->screen_transition[idx - 2].to;
    } else {
        /* On middle screen: use cursor position to decide which transition */
        if (state->pointer_x < MAX_SCREEN_COORD / 2) {
            border = &output->screen_transition[idx - 2].to;
        } else {
            border = &output->screen_transition[idx - 1].from;
        }
    }

    _get_border_position(state, border);
    save_config(state);

    /* Sync computer border to the other device so both have complete mapping data */
    if (is_computer_border)
        _sync_computer_border(state);
}

/* Hotkey handler - routes to the device with the mouse, which has the authoritative pointer position */
void screen_border_hotkey_handler(device_t *state, hid_keyboard_report_t *report) {
    if (state->mouse_connected) {
        _save_screen_border(state);
    } else {
        queue_packet(NULL, SYNC_BORDERS_MSG, 0);
    }
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

/* When pressed, enables the pong screensaver on active output */
void enable_screensaver_pong_hotkey_handler(device_t *state, hid_keyboard_report_t *report) {
    uint8_t desired_mode = state->config.output[BOARD_ROLE].screensaver.mode;

    /* If the user explicitly asks for pong screensaver to be active, ignore config and turn it on */
    if (desired_mode == DISABLED || desired_mode == JITTER)
        desired_mode = PONG;

    _screensaver_set(state, desired_mode);
}

/* When pressed, enables the jitter screensaver on active output */
void enable_screensaver_jitter_hotkey_handler(device_t *state, hid_keyboard_report_t *report) {
    uint8_t desired_mode = state->config.output[BOARD_ROLE].screensaver.mode;

    /* If the user explicitly asks for jitter screensaver to be active, ignore config and turn it on */
    if (desired_mode == DISABLED || desired_mode == PONG)
        desired_mode = JITTER;

    _screensaver_set(state, desired_mode);
}

/* When pressed, disables the screensaver on active output */
void disable_screensaver_hotkey_handler(device_t *state, hid_keyboard_report_t *report) {
    _screensaver_set(state, DISABLED);
}

/* Put the device into a special configuration mode */
void config_enable_hotkey_handler(device_t *state, hid_keyboard_report_t *report) {
    /* Enter config mode on the currently active output so the user sees the config UI */
    if (CURRENT_BOARD_IS_ACTIVE_OUTPUT)
        handle_enter_config_msg(NULL, state);
    else
        send_value(ENABLE, ENTER_CONFIG_MSG);
};


/* ==================================================== *
 * ==========  UART Message Handling Routines  ======== *
 * ==================================================== */

/* Function handles received keypresses from the other board */
void handle_keyboard_uart_msg(uart_packet_t *packet, device_t *state) {
    hid_keyboard_report_t *report = (hid_keyboard_report_t *)packet->data;
    hid_keyboard_report_t combined_report;

    /* Update the keyboard state for the remote device  */
    update_remote_kbd_state(state, report);

    /* Create a combined report from all device states */
    combine_kbd_states(state, &combined_report);

    /* Queue the combined report */
    queue_kbd_report(&combined_report, state);
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

/* Handle border syncing message - the other device is asking us to save the border.
 * Only process if we have the mouse connected (and thus the authoritative pointer position). */
void handle_sync_borders_msg(uart_packet_t *packet, device_t *state) {
    if (state->mouse_connected)
        _save_screen_border(state);
}

/* Enter configuration mode (called via message or directly) */
void handle_enter_config_msg(uart_packet_t *packet, device_t *state) {
    if (!state->config_mode_active) {
        watchdog_hw->scratch[5] = MAGIC_WORD_1;
        watchdog_hw->scratch[6] = MAGIC_WORD_2;
    }
    release_all_keys(state);
    state->reboot_requested = true;
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

    /* Track that we're sending firmware (for LED indication) */
    state->fw.last_request_time = time_us_32();

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
    uint16_t other_fw_version = packet->data16[0];
    uint32_t other_fw_crc = packet->data32[1];

    state->other_board_fw_crc = other_fw_crc;

    if (state->fw.upgrade_in_progress)
        return;

    if (other_fw_version > state->_running_fw.version) {
        dh_debug_printf("FW upgrade: version %u > %u\n", other_fw_version, state->_running_fw.version);
    } else if (other_fw_version == state->_running_fw.version &&
               other_fw_crc != state->fw_crc &&
               BOARD_ROLE == OUTPUT_B) {
        dh_debug_printf("FW upgrade: CRC mismatch (ours=%08lx, theirs=%08lx)\n", state->fw_crc, other_fw_crc);
    } else {
        return;
    }

    state->fw = (fw_upgrade_state_t) {
        .upgrade_in_progress = true,
        .byte_done = true,
        .address = 0,
        .checksum = 0xffffffff,
    };
}


/* ==================================================== *
 * ==============  Output Switch Routines  ============ *
 * ==================================================== */

/* Update output variable, set LED on/off and notify the other board so they are in sync. */
void set_active_output(device_t *state, uint8_t new_output) {
    state->active_output = new_output;
    restore_leds(state);
    send_value(new_output, OUTPUT_SELECT_MSG);

    /* If we were holding a key down and drag the mouse to another screen, the key gets stuck.
       Changing outputs = no more keypresses on the previous system. */
    release_all_keys(state);
}
