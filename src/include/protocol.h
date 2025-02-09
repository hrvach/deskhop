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
#pragma once

#include <stdint.h>

enum packet_type_e {
    KEYBOARD_REPORT_MSG  = 1,
    MOUSE_REPORT_MSG     = 2,
    OUTPUT_SELECT_MSG    = 3,
    FIRMWARE_UPGRADE_MSG = 4,
    MOUSE_ZOOM_MSG       = 5,
    KBD_SET_REPORT_MSG   = 6,
    SWITCH_LOCK_MSG      = 7,
    SYNC_BORDERS_MSG     = 8,
    FLASH_LED_MSG        = 9,
    WIPE_CONFIG_MSG      = 10,
    SCREENSAVER_MSG      = 11,
    HEARTBEAT_MSG        = 12,
    GAMING_MODE_MSG      = 13,
    CONSUMER_CONTROL_MSG = 14,
    SYSTEM_CONTROL_MSG   = 15,
    SAVE_CONFIG_MSG      = 18,
    REBOOT_MSG           = 19,
    GET_VAL_MSG          = 20,
    SET_VAL_MSG          = 21,
    GET_ALL_VALS_MSG     = 22,
    PROXY_PACKET_MSG     = 23,
    REQUEST_BYTE_MSG     = 24,
    RESPONSE_BYTE_MSG    = 25,
};

typedef enum {
    UINT8 = 0,
    UINT16 = 1,
    UINT32 = 2,
    UINT64 = 3,
    INT8 = 4,
    INT16 = 5,
    INT32 = 6,
    INT64 = 7,
    BOOL  = 8
} type_e;

/*==============================================================================
 *  API Request Data Structure, defines offset within struct, length, type,
    write permissions and packet ID type (index)
 *==============================================================================*/

typedef struct {
    uint32_t idx;
    bool readonly;
    type_e type;
    uint32_t len;
    size_t offset;
} field_map_t;
