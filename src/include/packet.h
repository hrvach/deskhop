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
#include "protocol.h"


/*==============================================================================
 *  Constants
 *==============================================================================*/

/* Preamble */
#define START1        0xAA
#define START2        0x55
#define START_LENGTH  2

/* Packet Queue Definitions  */
#define UART_QUEUE_LENGTH  256
#define HID_QUEUE_LENGTH   128
#define KBD_QUEUE_LENGTH   128
#define MOUSE_QUEUE_LENGTH 512

/* Packet Lengths and Offsets */
#define PACKET_LENGTH          (TYPE_LENGTH + PACKET_DATA_LENGTH + CHECKSUM_LENGTH)
#define RAW_PACKET_LENGTH      (START_LENGTH + PACKET_LENGTH)

#define TYPE_LENGTH             1
#define PACKET_DATA_LENGTH      8 // For simplicity, all packet types are the same length
#define CHECKSUM_LENGTH         1

#define KEYARRAY_BIT_OFFSET     16
#define KEYS_IN_USB_REPORT      6
#define KBD_REPORT_LENGTH       8
#define MOUSE_REPORT_LENGTH     8
#define CONSUMER_CONTROL_LENGTH 4
#define SYSTEM_CONTROL_LENGTH   1
#define MODIFIER_BIT_LENGTH     8

/*==============================================================================
 *  Data Structures
 *==============================================================================*/

 typedef struct {
    uint8_t type;     // Enum field describing the type of packet
    union {
        uint8_t data[8];      // Data goes here (type + payload + checksum)
        uint16_t data16[4];   // We can treat it as 4 16-byte chunks
        uint32_t data32[2];   // We can treat it as 2 32-byte chunks
    };
    uint8_t checksum; // Checksum, a simple XOR-based one
} __attribute__((packed)) uart_packet_t;
