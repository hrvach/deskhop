/*
 * This file is part of DeskHop (https://github.com/hrvach/deskhop).
 * Copyright (c) 2024 Hrvoje Cavrak
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * See the file LICENSE for the full license text.
 */
#pragma once

#include "main.h"
#include "tusb.h"

/*==============================================================================
 *  Constants
 *==============================================================================*/

#define HID_DEFAULT_NUM_COLLECTIONS 16
#define HID_MAX_USAGES              128
#define MAX_CC_BUTTONS              16
#define MAX_DEVICES                 3
#define MAX_INTERFACES              6
#define MAX_KEYS                    32
#define MAX_REPORTS                 24
#define MAX_SYS_BUTTONS             8

/*==============================================================================
 *  Data Structures
 *==============================================================================*/

/* Counts how many collection starts and ends we've seen, when they equalize
   (and not zero), we are at the end of a block */
typedef struct {
    uint8_t start;
    uint8_t end;
} collection_t;

/* Header byte is unpacked to size/type/tag using this struct */
typedef struct TU_ATTR_PACKED {
    uint8_t size : 2;
    uint8_t type : 2;
    uint8_t tag : 4;
} header_t;

/* We store a header block and corresponding data in an array of these
   to avoid having to use numerous switch-case checks */
typedef struct {
    header_t hdr;
    uint32_t val;
} item_t;

typedef enum {
    DATA = 0,
    CONSTANT,
    ARRAY,
    VARIABLE,
    ABSOLUTE_DATA,
    RELATIVE_DATA,
    NO_WRAP,
    WRAP,
    LINEAR,
    NONLINEAR,
} data_type_e;

// Extended precision mouse movement information
typedef struct {
    int32_t move_x;
    int32_t move_y;
    int32_t wheel;
    int32_t pan;
    int32_t buttons;
} mouse_values_t;

/* Describes where can we find a value in a HID report */
typedef struct TU_ATTR_PACKED {
    uint16_t offset;     // In bits
    uint16_t offset_idx; // In bytes
    uint16_t size;       // In bits

    int32_t usage_min;
    int32_t usage_max;

    uint8_t item_type;
    uint8_t data_type;

    uint8_t report_id;
    uint16_t global_usage;
    uint16_t usage_page;
    uint16_t usage;
} report_val_t;

/* Defines information about HID report format for the mouse. */
typedef struct {
    report_val_t buttons;
    report_val_t move_x;
    report_val_t move_y;
    report_val_t wheel;
    report_val_t pan;

    uint8_t report_id;

    bool is_found;
    bool uses_report_id;
} mouse_t;

typedef struct hid_interface_t hid_interface_t;
typedef void (*process_report_f)(uint8_t *, int, uint8_t, hid_interface_t *);

/* Defines information about HID report format for the keyboard. */
typedef struct {
    report_val_t modifier;
    report_val_t nkro;
    uint16_t cc_array[MAX_CC_BUTTONS];
    uint16_t sys_array[MAX_SYS_BUTTONS];
    bool key_array[MAX_KEYS];

    uint8_t report_id;
    uint8_t key_array_idx;

    bool uses_report_id;
    bool is_found;
    bool is_nkro;
} keyboard_t;

typedef struct {
    report_val_t val;
    uint8_t report_id;
    bool is_variable;
    bool is_array;
} report_t;

struct hid_interface_t {
    keyboard_t keyboard;
    mouse_t mouse;
    report_t consumer;
    report_t system;
    process_report_f report_handler[MAX_REPORTS];
    uint8_t protocol;
    bool uses_report_id;
};

typedef struct {
    report_val_t *map;
    int map_index; /* Index of the current element we've found */
    int report_id; /* Report ID of the current section we're parsing */

    uint32_t usage_count;
    uint32_t offset_in_bits;
    uint16_t usages[HID_MAX_USAGES];
    uint16_t *p_usage;
    uint16_t global_usage;

    collection_t collection;

    /* as tag is 4 bits, there can be 16 different tags in global header type */
    item_t globals[16];

    /* as tag is 4 bits, there can be 16 different tags in local header type */
    item_t locals[16];
} parser_state_t;

///////////////
