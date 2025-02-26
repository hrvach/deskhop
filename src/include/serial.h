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

#include <hardware/uart.h>
#include "structs.h"

/*==============================================================================
 *  Constants
 *==============================================================================*/

#define SERIAL_BAUDRATE   3686400
#define SERIAL_DATA_BITS  8
#define SERIAL_PARITY     UART_PARITY_NONE
#define SERIAL_STOP_BITS  1
#define SERIAL_UART       uart1

/*==============================================================================
 *  Serial Communication Functions
 *==============================================================================*/

bool get_packet_from_buffer(device_t *);
void process_packet(uart_packet_t *, device_t *);
void queue_packet(const uint8_t *, enum packet_type_e, int);
void send_value(const uint8_t, enum packet_type_e);
void write_raw_packet(uint8_t *, uart_packet_t *);
