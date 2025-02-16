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
#include <hardware/flash.h>

/*==============================================================================
 *  Firmware Metadata
 *==============================================================================*/

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint32_t checksum;
} firmware_metadata_t;

extern firmware_metadata_t _firmware_metadata;
#define FIRMWARE_METADATA_MAGIC   0xf00d

/*==============================================================================
 *  Firmware Transfer Packet
 *==============================================================================*/

typedef struct {
    uint8_t cmd;          // Byte 0 = command
    uint16_t page_number; // Bytes 1-2 = page number
    union {
        uint8_t offset;   // Byte 3 = offset
        uint8_t checksum; // In write packets, it's checksum
    };
    uint8_t data[4]; // Bytes 4-7 = data
} fw_packet_t;

/*==============================================================================
 *  Flash Memory Layout
 *==============================================================================*/

#define RUNNING_FIRMWARE_SLOT     0
#define STAGING_FIRMWARE_SLOT     1
#define STAGING_PAGES_CNT         1024
#define STAGING_IMAGE_SIZE        STAGING_PAGES_CNT * FLASH_PAGE_SIZE

/*==============================================================================
*  Lookup Tables
*==============================================================================*/

extern const uint32_t crc32_lookup_table[];

/*==============================================================================
 *  UF2 Firmware Format Structure
 *==============================================================================*/
typedef struct {
    uint32_t magicStart0;
    uint32_t magicStart1;
    uint32_t flags;
    uint32_t targetAddr;
    uint32_t payloadSize;
    uint32_t blockNo;
    uint32_t numBlocks;
    uint32_t fileSize;
    uint8_t data[476];
    uint32_t magicEnd;
} uf2_t;

#define UF2_MAGIC_START0 0x0A324655
#define UF2_MAGIC_START1 0x9E5D5157
#define UF2_MAGIC_END    0x0AB16F30
