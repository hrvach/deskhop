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

#include "main.h"

/*==============================================================================
 *  Function Pointer Definitions
 *==============================================================================*/

typedef void (*value_handler_f)(report_val_t *, report_val_t *, hid_interface_t *);
typedef uint8_t *(*report_id_getter_f)(hid_interface_t *iface);

/*==============================================================================
 *  Data Structures
 *==============================================================================*/

typedef struct {
    int global_usage;
    int usage_page;
    int usage;
    report_val_t *dst;
    value_handler_f handler;
    process_report_f receiver;
    report_id_getter_f get_id;
} usage_map_t;
