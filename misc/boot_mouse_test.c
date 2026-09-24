/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (c) 2026 Derek Reynolds */

#include <assert.h>

#include "boot_mouse.h"

int main(void) {
    uint8_t report[3];

    encode_boot_mouse_report(report, 5, 0, -1);
    assert(report[0] == 5 && report[1] == 0 && report[2] == 255);

    encode_boot_mouse_report(report, 1, 300, -300);
    assert(report[0] == 1 && report[1] == 127 && report[2] == 128);
}
