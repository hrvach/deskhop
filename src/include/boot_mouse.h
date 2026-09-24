/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (c) 2026 Derek Reynolds */

#pragma once

#include <stdint.h>

static inline void encode_boot_mouse_report(uint8_t report[3], uint8_t buttons, int32_t x, int32_t y) {
    report[0] = buttons;
    report[1] = (uint8_t)(int8_t)(x < INT8_MIN ? INT8_MIN : x > INT8_MAX ? INT8_MAX : x);
    report[2] = (uint8_t)(int8_t)(y < INT8_MIN ? INT8_MIN : y > INT8_MAX ? INT8_MAX : y);
}
