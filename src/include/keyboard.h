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

#include "structs.h"
#include "hid_parser.h"

/*==============================================================================
 *  Data Extraction
 *==============================================================================*/

int32_t  extract_bit_variable(report_val_t *, uint8_t *, int, uint8_t *);
int32_t  extract_kbd_data(uint8_t *, int, uint8_t, hid_interface_t *, hid_keyboard_report_t *);

/*==============================================================================
 *  Hotkey Handling
 *==============================================================================*/

bool check_specific_hotkey(hotkey_combo_t, const hid_keyboard_report_t *);

/*==============================================================================
 *  Keyboard State Management
 *==============================================================================*/
void     update_kbd_state(device_t *, hid_keyboard_report_t *, uint8_t);
void     combine_kbd_states(device_t *, hid_keyboard_report_t *);

/*==============================================================================
 *  Keyboard Report Processing
 *==============================================================================*/
bool     key_in_report(uint8_t, const hid_keyboard_report_t *);
void     process_consumer_report(uint8_t *, int, uint8_t, hid_interface_t *);
void     process_keyboard_report(uint8_t *, int, uint8_t, hid_interface_t *);
void     process_system_report(uint8_t *, int, uint8_t, hid_interface_t *);
void     queue_cc_packet(uint8_t *, device_t *);
void     queue_kbd_report(hid_keyboard_report_t *, device_t *);
void     queue_system_packet(uint8_t *, device_t *);
void     release_all_keys(device_t *);
void     send_consumer_control(uint8_t *, device_t *);
void     send_key(hid_keyboard_report_t *, device_t *);

/* ==================================================== *
 * Map hotkeys to alternative layouts
 * ==================================================== */

/* DVORAK */
#define DVORAK_HID_KEY_A HID_KEY_A
#define DVORAK_HID_KEY_B HID_KEY_N
#define DVORAK_HID_KEY_C HID_KEY_I
#define DVORAK_HID_KEY_D HID_KEY_H
#define DVORAK_HID_KEY_E HID_KEY_D
#define DVORAK_HID_KEY_F HID_KEY_Y
#define DVORAK_HID_KEY_G HID_KEY_U
#define DVORAK_HID_KEY_H HID_KEY_J
#define DVORAK_HID_KEY_I HID_KEY_G
#define DVORAK_HID_KEY_J HID_KEY_C
#define DVORAK_HID_KEY_K HID_KEY_V
#define DVORAK_HID_KEY_L HID_KEY_P
#define DVORAK_HID_KEY_M HID_KEY_M
#define DVORAK_HID_KEY_N HID_KEY_L
#define DVORAK_HID_KEY_O HID_KEY_S
#define DVORAK_HID_KEY_P HID_KEY_R
#define DVORAK_HID_KEY_Q HID_KEY_X
#define DVORAK_HID_KEY_R HID_KEY_O
#define DVORAK_HID_KEY_S HID_KEY_SEMICOLON
#define DVORAK_HID_KEY_T HID_KEY_K
#define DVORAK_HID_KEY_U HID_KEY_F
#define DVORAK_HID_KEY_V HID_KEY_PERIOD
#define DVORAK_HID_KEY_W HID_KEY_COMMA
#define DVORAK_HID_KEY_X HID_KEY_B
#define DVORAK_HID_KEY_Y HID_KEY_T
#define DVORAK_HID_KEY_Z HID_KEY_SLASH

/* COLEMAK */
#define COLEMAK_HID_KEY_A HID_KEY_A
#define COLEMAK_HID_KEY_B HID_KEY_B
#define COLEMAK_HID_KEY_C HID_KEY_C
#define COLEMAK_HID_KEY_D HID_KEY_G
#define COLEMAK_HID_KEY_E HID_KEY_K
#define COLEMAK_HID_KEY_F HID_KEY_E
#define COLEMAK_HID_KEY_G HID_KEY_T
#define COLEMAK_HID_KEY_H HID_KEY_H
#define COLEMAK_HID_KEY_I HID_KEY_L
#define COLEMAK_HID_KEY_J HID_KEY_Y
#define COLEMAK_HID_KEY_K HID_KEY_N
#define COLEMAK_HID_KEY_L HID_KEY_U
#define COLEMAK_HID_KEY_M HID_KEY_M
#define COLEMAK_HID_KEY_N HID_KEY_J
#define COLEMAK_HID_KEY_O HID_KEY_SEMICOLON
#define COLEMAK_HID_KEY_P HID_KEY_R
#define COLEMAK_HID_KEY_Q HID_KEY_Q
#define COLEMAK_HID_KEY_R HID_KEY_S
#define COLEMAK_HID_KEY_S HID_KEY_D
#define COLEMAK_HID_KEY_T HID_KEY_F
#define COLEMAK_HID_KEY_U HID_KEY_I
#define COLEMAK_HID_KEY_V HID_KEY_V
#define COLEMAK_HID_KEY_W HID_KEY_W
#define COLEMAK_HID_KEY_X HID_KEY_X
#define COLEMAK_HID_KEY_Y HID_KEY_O
#define COLEMAK_HID_KEY_Z HID_KEY_Z

/* QWERTY needs no change */
#if KEYBOARD_LAYOUT == 0
#define KBD_REMAP(key) key

/* For DVORAK we prepend DVORAK_ and reference the definitions above */
#elif KEYBOARD_LAYOUT == 1
#define KBD_REMAP(key) DVORAK_##key

/* For COLEMAK we prepend COLEMAK_ and reference the definitions above */
#elif KEYBOARD_LAYOUT == 2
#define KBD_REMAP(key) COLEMAK_##key
#endif
