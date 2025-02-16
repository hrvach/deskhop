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

#include <hardware/watchdog.h>

#define WATCHDOG_TIMEOUT        500                     // In milliseconds => needs to be reset at least every 200ms
#define WATCHDOG_PAUSE_ON_DEBUG 1                       // When using a debugger, disable watchdog
#define CORE1_HANG_TIMEOUT_US   WATCHDOG_TIMEOUT * 1000 // Convert to microseconds

#define MAGIC_WORD_1 0xdeadf00f // When these are set, we'll boot to configuration mode
#define MAGIC_WORD_2 0x00c0ffee
