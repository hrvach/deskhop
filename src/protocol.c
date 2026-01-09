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

const field_map_t api_field_map[] = {
/* Index, Rdonly, Type, Len, Offset in struct */
    { 0,  true,  UINT8,  1, offsetof(device_t, active_output) },
    { 1,  true,  INT16,  2, offsetof(device_t, pointer_x) },
    { 2,  true,  INT16,  2, offsetof(device_t, pointer_y) },
    { 3,  true,  INT16,  2, offsetof(device_t, mouse_buttons) },

    /* Output A */
    { 10, false, UINT32, 4, offsetof(device_t, config.output[0].number) },
    { 11, false, UINT32, 4, offsetof(device_t, config.output[0].screen_count) },
    { 12, false, INT32,  4, offsetof(device_t, config.output[0].speed_x) },
    { 13, false, INT32,  4, offsetof(device_t, config.output[0].speed_y) },
    { 14, false, INT32,  4, offsetof(device_t, config.output[0].border.top) },
    { 15, false, INT32,  4, offsetof(device_t, config.output[0].border.bottom) },
    { 16, false, UINT8,  1, offsetof(device_t, config.output[0].os) },
    { 17, false, UINT8,  1, offsetof(device_t, config.output[0].pos) },
    { 18, false, UINT8,  1, offsetof(device_t, config.output[0].mouse_park_pos) },
    { 19, false, UINT8,  1, offsetof(device_t, config.output[0].screensaver.mode) },
    { 20, false, UINT8,  1, offsetof(device_t, config.output[0].screensaver.only_if_inactive) },

    /* Until we increase the payload size from 8 bytes, clamp to avoid exceeding the field size */
    { 21, false, UINT64, 7, offsetof(device_t, config.output[0].screensaver.idle_time_us) },
    { 22, false, UINT64, 7, offsetof(device_t, config.output[0].screensaver.max_time_us) },

    /* Output A screen transitions */
    { 23, false, INT32, 4, offsetof(device_t, config.output[0].screen_transition[0].from.top) },
    { 24, false, INT32, 4, offsetof(device_t, config.output[0].screen_transition[0].from.bottom) },
    { 25, false, INT32, 4, offsetof(device_t, config.output[0].screen_transition[0].to.top) },
    { 26, false, INT32, 4, offsetof(device_t, config.output[0].screen_transition[0].to.bottom) },
    { 27, false, INT32, 4, offsetof(device_t, config.output[0].screen_transition[1].from.top) },
    { 28, false, INT32, 4, offsetof(device_t, config.output[0].screen_transition[1].from.bottom) },
    { 29, false, INT32, 4, offsetof(device_t, config.output[0].screen_transition[1].to.top) },
    { 30, false, INT32, 4, offsetof(device_t, config.output[0].screen_transition[1].to.bottom) },

    /* Output B */
    { 40, false, UINT32, 4, offsetof(device_t, config.output[1].number) },
    { 41, false, UINT32, 4, offsetof(device_t, config.output[1].screen_count) },
    { 42, false, INT32,  4, offsetof(device_t, config.output[1].speed_x) },
    { 43, false, INT32,  4, offsetof(device_t, config.output[1].speed_y) },
    { 44, false, INT32,  4, offsetof(device_t, config.output[1].border.top) },
    { 45, false, INT32,  4, offsetof(device_t, config.output[1].border.bottom) },
    { 46, false, UINT8,  1, offsetof(device_t, config.output[1].os) },
    { 47, false, UINT8,  1, offsetof(device_t, config.output[1].pos) },
    { 48, false, UINT8,  1, offsetof(device_t, config.output[1].mouse_park_pos) },
    { 49, false, UINT8,  1, offsetof(device_t, config.output[1].screensaver.mode) },
    { 50, false, UINT8,  1, offsetof(device_t, config.output[1].screensaver.only_if_inactive) },
    { 51, false, UINT64, 7, offsetof(device_t, config.output[1].screensaver.idle_time_us) },
    { 52, false, UINT64, 7, offsetof(device_t, config.output[1].screensaver.max_time_us) },

    /* Output B screen transitions */
    { 53, false, INT32, 4, offsetof(device_t, config.output[1].screen_transition[0].from.top) },
    { 54, false, INT32, 4, offsetof(device_t, config.output[1].screen_transition[0].from.bottom) },
    { 55, false, INT32, 4, offsetof(device_t, config.output[1].screen_transition[0].to.top) },
    { 56, false, INT32, 4, offsetof(device_t, config.output[1].screen_transition[0].to.bottom) },
    { 57, false, INT32, 4, offsetof(device_t, config.output[1].screen_transition[1].from.top) },
    { 58, false, INT32, 4, offsetof(device_t, config.output[1].screen_transition[1].from.bottom) },
    { 59, false, INT32, 4, offsetof(device_t, config.output[1].screen_transition[1].to.top) },
    { 60, false, INT32, 4, offsetof(device_t, config.output[1].screen_transition[1].to.bottom) },

    /* Common config */
    { 70, false, UINT32, 4, offsetof(device_t, config.version) },
    { 71, false, UINT8,  1, offsetof(device_t, config.force_mouse_boot_mode) },
    { 72, false, UINT8,  1, offsetof(device_t, config.force_kbd_boot_protocol) },
    { 73, false, UINT8,  1, offsetof(device_t, config.kbd_led_as_indicator) },
    { 74, false, UINT8,  1, offsetof(device_t, config.hotkey_toggle) },
    { 75, false, UINT8,  1, offsetof(device_t, config.enable_acceleration) },
    { 76, false, UINT8,  1, offsetof(device_t, config.enforce_ports) },
    { 77, false, UINT16, 2, offsetof(device_t, config.jump_threshold) },

    /* Firmware */
    { 78, true,  UINT16, 2, offsetof(device_t, _running_fw.version) },
    { 79, true,  UINT32, 4, offsetof(device_t, _running_fw.checksum) },

    { 80, true,  UINT8,  1, offsetof(device_t, keyboard_connected) },
    { 81, true,  UINT8,  1, offsetof(device_t, switch_lock) },
    { 82, true,  UINT8,  1, offsetof(device_t, relative_mouse) },
};

const field_map_t* get_field_map_entry(uint32_t index) {
    for (unsigned int i = 0; i < ARRAY_SIZE(api_field_map); i++) {
        if (api_field_map[i].idx == index) {
            return &api_field_map[i];
        }
    }

    return NULL;
}


const field_map_t* get_field_map_index(uint32_t index) {
    /* Clamp potential overflows to last element. */
    if (index >= ARRAY_SIZE(api_field_map))
        index = ARRAY_SIZE(api_field_map) - 1;

    return &api_field_map[index];
}

size_t get_field_map_length(void) {
    return ARRAY_SIZE(api_field_map);
}

void _queue_packet(uint8_t *payload, device_t *state, uint8_t type, uint8_t len, uint8_t id, uint8_t inst) {
    hid_generic_pkt_t generic_packet = {
        .instance = inst,
        .report_id = id,
        .type = type,
        .len = len,
    };

    memcpy(generic_packet.data, payload, len);
    queue_try_add(&state->hid_queue_out, &generic_packet);
}

void queue_cfg_packet(uart_packet_t *packet, device_t *state) {
    uint8_t raw_packet[RAW_PACKET_LENGTH];
    write_raw_packet(raw_packet, packet);
    _queue_packet(raw_packet, state, 0, RAW_PACKET_LENGTH, REPORT_ID_VENDOR, ITF_NUM_HID_VENDOR);
}

void queue_cc_packet(uint8_t *payload, device_t *state) {
    _queue_packet(payload, state, 1, CONSUMER_CONTROL_LENGTH, REPORT_ID_CONSUMER, ITF_NUM_HID);
}

void queue_system_packet(uint8_t *payload, device_t *state) {
    _queue_packet(payload, state, 2, SYSTEM_CONTROL_LENGTH, REPORT_ID_SYSTEM, ITF_NUM_HID);
}
