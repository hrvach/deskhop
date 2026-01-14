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
 *  Constants
 *==============================================================================*/

#define MAX_SCREEN_COORD 32767
#define MIN_SCREEN_COORD 0
#define MAX_SCREEN_COUNT 3  // Max physical monitors per computer

/*==============================================================================
 *  Data Structures
 *==============================================================================*/

typedef struct {
    int16_t start;  // Range start (Y for horizontal layouts, X for vertical)
    int16_t end;    // Range end
} border_size_t;

typedef struct {
    border_size_t from;  // Range on lower-indexed screen to allow transition
    border_size_t to;    // Range on higher-indexed screen to allow transition back
} screen_transition_t;

typedef enum {
    LAYOUT_HORIZONTAL = 0,  // Side-by-side arrangement
    LAYOUT_VERTICAL = 1     // Stacked arrangement (top-to-bottom)
} layout_t;

typedef struct {
    uint8_t mode;
    uint8_t only_if_inactive;
    uint64_t idle_time_us;
    uint64_t max_time_us;
} screensaver_t;

typedef struct {
    uint32_t number;           // Number of this output (e.g. OUTPUT_A = 0 etc)
    uint32_t screen_count;     // How many monitors per output (e.g. Output A is Windows with 3 monitors)
    uint32_t screen_index;     // Current active screen
    int32_t speed_x;           // Mouse speed per output, in direction X
    int32_t speed_y;           // Mouse speed per output, in direction Y
    screen_transition_t screen_transition[MAX_SCREEN_COUNT - 1];  // Ranges for screen transitions
    uint8_t os;                // Operating system on this output
    uint8_t pos;               // Screen position on this output (LEFT/RIGHT or TOP/BOTTOM)
    uint8_t mouse_park_pos;    // Where the mouse goes after switch
    layout_t monitor_layout;   // How this computer's monitors are arranged
    uint8_t border_monitor_index;  // For vertical layouts: which monitor can switch to other computer
    screensaver_t screensaver; // Screensaver parameters for this output
} output_t;
