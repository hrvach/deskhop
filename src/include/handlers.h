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
#include "keyboard.h"

/*==============================================================================
 *  Hotkey Handlers
 *  These handlers are invoked when specific hotkey combinations are detected.
 *==============================================================================*/

void config_enable_hotkey_handler(device_t *, hid_keyboard_report_t *);
void disable_screensaver_hotkey_handler(device_t *, hid_keyboard_report_t *);
void enable_screensaver_hotkey_handler(device_t *, hid_keyboard_report_t *);
void fw_upgrade_hotkey_handler_A(device_t *, hid_keyboard_report_t *);
void fw_upgrade_hotkey_handler_B(device_t *, hid_keyboard_report_t *);
void mouse_zoom_hotkey_handler(device_t *, hid_keyboard_report_t *);
void output_config_hotkey_handler(device_t *, hid_keyboard_report_t *);
void output_toggle_hotkey_handler(device_t *, hid_keyboard_report_t *);
void screen_border_hotkey_handler(device_t *, hid_keyboard_report_t *);
void screenlock_hotkey_handler(device_t *, hid_keyboard_report_t *);
void switchlock_hotkey_handler(device_t *, hid_keyboard_report_t *);
void toggle_gaming_mode_handler(device_t *, hid_keyboard_report_t *);
void wipe_config_hotkey_handler(device_t *, hid_keyboard_report_t *);

/*==============================================================================
 *  UART Message Handlers
 *  These handlers process incoming messages received over the UART interface.
 *==============================================================================*/

void handle_api_msgs(uart_packet_t *, device_t *);
void handle_api_read_all_msg(uart_packet_t *, device_t *);
void handle_consumer_control_msg(uart_packet_t *, device_t *);
void handle_flash_led_msg(uart_packet_t *, device_t *);
void handle_fw_upgrade_msg(uart_packet_t *, device_t *);
void handle_toggle_gaming_msg(uart_packet_t *, device_t *);
void handle_heartbeat_msg(uart_packet_t *, device_t *);
void handle_keyboard_uart_msg(uart_packet_t *, device_t *);
void handle_mouse_abs_uart_msg(uart_packet_t *, device_t *);
void handle_mouse_zoom_msg(uart_packet_t *, device_t *);
void handle_output_select_msg(uart_packet_t *, device_t *);
void handle_proxy_msg(uart_packet_t *, device_t *);
void handle_read_config_msg(uart_packet_t *, device_t *);
void handle_reboot_msg(uart_packet_t *, device_t *);
void handle_request_byte_msg(uart_packet_t *, device_t *);
void handle_response_byte_msg(uart_packet_t *, device_t *);
void handle_save_config_msg(uart_packet_t *, device_t *);
void handle_screensaver_msg(uart_packet_t *, device_t *);
void handle_set_report_msg(uart_packet_t *, device_t *);
void handle_switch_lock_msg(uart_packet_t *, device_t *);
void handle_sync_borders_msg(uart_packet_t *, device_t *);
void handle_wipe_config_msg(uart_packet_t *, device_t *);
void handle_write_fw_msg(uart_packet_t *, device_t *);

/*==============================================================================
 *  Output Control
 *  Functions related to managing the active output.
 *==============================================================================*/

void set_active_output(device_t *, uint8_t);
