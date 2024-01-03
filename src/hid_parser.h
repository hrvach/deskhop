/*
 * This file is part of DeskHop (https://github.com/hrvach/deskhop).
 * Copyright (c) 2024 Hrvoje Cavrak
 *
 * Based on the TinyUSB HID parser routine and the amazing USB2N64
 * adapter (https://github.com/pdaxrom/usb2n64-adapter)
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
#pragma once
#define MAX_REPORTS 32

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
} globals_t;

// Extended precision mouse movement information
typedef struct {
    int32_t move_x;
    int32_t move_y;
    int32_t wheel;
    int32_t pan;
    uint32_t buttons;
} mouse_values_t;

/* Describes where can we find a value in a HID report */
typedef struct {
    uint16_t offset;  // In bits
    uint8_t size;     // In bits
    int32_t min;
    int32_t max;
} report_val_t;

/* Defines information about HID report format for the mouse. */
typedef struct {
    report_val_t buttons;

    report_val_t move_x;
    report_val_t move_y;

    report_val_t wheel;

    bool uses_report_id;
    uint8_t report_id;
    uint8_t protocol;
} mouse_t;

/* For each element type we're interested in there is an entry
in an array of these, defining its usage and in case matched, where to
store the data. */
typedef struct {
    uint8_t report_usage;
    uint8_t usage_page;
    uint8_t usage;
    report_val_t* element;
} usage_map_t;
