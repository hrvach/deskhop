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

/*==============================================================================
 *  Board and Output Roles
 *==============================================================================*/

#define OUTPUT_A 0
#define OUTPUT_B 1

/*==============================================================================
 *  HID Interface Numbers
 *==============================================================================*/

#define ITF_NUM_HID        0
#define ITF_NUM_HID_REL_M  1
#define ITF_NUM_HID_VENDOR 1
#define ITF_NUM_MSC        2

/*==============================================================================
 *  Mouse Modes
 *==============================================================================*/

#define ABSOLUTE 0
#define RELATIVE 1
#define TOUCH 2

/*==============================================================================
 *  Boolean States
 *==============================================================================*/

#define ENABLE  1
#define DISABLE 0

/*==============================================================================
 *  Numerical Constants
 *==============================================================================*/

#define CONFIG_MODE_TIMEOUT 300000000 // 5 minutes into the future
#define JITTER_DISTANCE 2
#define MOUSE_BOOT_REPORT_LEN 4
#define MOUSE_ZOOM_SCALING_FACTOR 2
#define NUM_SCREENS 2

/*==============================================================================
 *  Utility Macros
 *==============================================================================*/

#define _HZ(x) ((uint64_t)((1000000) / (x)))
#define _MS(x) (x * 1000)
#define _SEC(x) (x * 1000000)
#define _TOP()  0
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define CURRENT_BOARD_IS_ACTIVE_OUTPUT (global_state.active_output == global_state.board_role)
