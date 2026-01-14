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

_Static_assert(MAX_SCREEN_COUNT == 3,
    "Update screen_transition defaults in defaults.c when MAX_SCREEN_COUNT changes");

/* Default configuration */
const config_t default_config = {
    .magic_header = 0xB00B1E5,
    .version = CURRENT_CONFIG_VERSION,
    .output[OUTPUT_A] =
        {
            .number = OUTPUT_A,
            .speed_x = MOUSE_SPEED_A_FACTOR_X,
            .speed_y = MOUSE_SPEED_A_FACTOR_Y,
            .screen_transition = {
                [0] = { .from = { .start = 0, .end = MAX_SCREEN_COORD },
                        .to   = { .start = 0, .end = MAX_SCREEN_COORD } },
                [1] = { .from = { .start = 0, .end = MAX_SCREEN_COORD },
                        .to   = { .start = 0, .end = MAX_SCREEN_COORD } },
            },
            .screen_count = 1,
            .screen_index = 1,
            .os = OUTPUT_A_OS,
            .pos = RIGHT,
            .monitor_layout = LAYOUT_HORIZONTAL,
            .border_monitor_index = 1,
            .screensaver = {
                .mode = SCREENSAVER_A_MODE,
                .only_if_inactive = SCREENSAVER_A_ONLY_IF_INACTIVE,
                .idle_time_us = (uint64_t)SCREENSAVER_A_IDLE_TIME_SEC * 1000000,
                .max_time_us = (uint64_t)SCREENSAVER_A_MAX_TIME_SEC * 1000000,
            }
        },
    .output[OUTPUT_B] =
        {
            .number = OUTPUT_B,
            .speed_x = MOUSE_SPEED_B_FACTOR_X,
            .speed_y = MOUSE_SPEED_B_FACTOR_Y,
            .screen_transition = {
                [0] = { .from = { .start = 0, .end = MAX_SCREEN_COORD },
                        .to   = { .start = 0, .end = MAX_SCREEN_COORD } },
                [1] = { .from = { .start = 0, .end = MAX_SCREEN_COORD },
                        .to   = { .start = 0, .end = MAX_SCREEN_COORD } },
            },
            .screen_count = 1,
            .screen_index = 1,
            .os = OUTPUT_B_OS,
            .pos = LEFT,
            .monitor_layout = LAYOUT_HORIZONTAL,
            .border_monitor_index = 1,
            .screensaver = {
                .mode = SCREENSAVER_B_MODE,
                .only_if_inactive = SCREENSAVER_B_ONLY_IF_INACTIVE,
                .idle_time_us = (uint64_t)SCREENSAVER_B_IDLE_TIME_SEC * 1000000,
                .max_time_us = (uint64_t)SCREENSAVER_B_MAX_TIME_SEC * 1000000,
            }
        },
    .enforce_ports = ENFORCE_PORTS,
    .force_kbd_boot_protocol = ENFORCE_KEYBOARD_BOOT_PROTOCOL,
    .force_mouse_boot_mode = false,
    .enable_acceleration = ENABLE_ACCELERATION,
    .hotkey_toggle = HOTKEY_TOGGLE,
    .kbd_led_as_indicator = KBD_LED_AS_INDICATOR,
    .jump_threshold = JUMP_THRESHOLD,
    .hold_threshold_ms = HOTKEY_HOLD_THRESHOLD_MS,
    .computer_border = {
        .from = { .start = 0, .end = MAX_SCREEN_COORD },
        .to   = { .start = 0, .end = MAX_SCREEN_COORD },
    },
};
