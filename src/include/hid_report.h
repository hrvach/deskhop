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

#include "main.h"

typedef void (*value_handler_f)(report_val_t *, report_val_t *, hid_interface_t *);

/* For each element type we're interested in there is an entry
in an array of these, defining its usage and in case matched, where to
store the data. */
typedef struct {
    int global_usage;
    int usage_page;
    int usage;
    uint8_t *id;
    report_val_t *dst;
    value_handler_f handler;
    process_report_f receiver;
} usage_map_t;
