/*
 * This file is part of DeskHop (https://github.com/hrvach/deskhop).
 * Copyright (c) 2024 Hrvoje Cavrak
 *
 * Based on the TinyUSB example by Ha Thach.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "main.h"

#define NUMBER_OF_BLOCKS 4096
#define ACTUAL_NUMBER_OF_BLOCKS 128
#define BLOCK_SIZE       512

void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]) {
    strcpy((char *)vendor_id, "DeskHop");
    strcpy((char *)product_id, "Config Mode");
    strcpy((char *)product_rev, "1.0");
}

bool tud_msc_test_unit_ready_cb(uint8_t lun) {
    return true;
}

void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count, uint16_t *block_size) {
    *block_count = NUMBER_OF_BLOCKS;
    *block_size  = BLOCK_SIZE;
}

bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject) {
    return true;
}

/* Return the requested data, or -1 if out-of-bounds */
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize) {
    const uint8_t *addr = &ADDR_DISK_IMAGE[lba * BLOCK_SIZE + offset];

    if (lba >= NUMBER_OF_BLOCKS)
        return -1;

    /* We lie about the image size - actually it's 64 kB, not 512 kB, so if we're out of bounds, return zeros */
    else if (lba >= ACTUAL_NUMBER_OF_BLOCKS) 
        memset(buffer, 0x00, bufsize);

    else 
        memcpy(buffer, addr, bufsize);
    
    return (int32_t)bufsize;
}

/* We're writable, so return true */
bool tud_msc_is_writable_cb(uint8_t lun) {
    return true;
}

/* Simple firmware write routine, we get 512-byte uf2 blocks with 256 byte payload */
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize) {
    uf2_t *uf2 = (uf2_t *)&buffer[0];
    bool is_final_block = uf2->blockNo == (STAGING_IMAGE_SIZE / FLASH_PAGE_SIZE) - 1;    
    uint32_t flash_addr = (uint32_t)ADDR_FW_RUNNING + uf2->blockNo * FLASH_PAGE_SIZE - XIP_BASE;   
    
    if (lba >= NUMBER_OF_BLOCKS)
        return -1;

    /* If we're not detecting UF2 magic constants, we have nothing to do... */
    if (uf2->magicStart0 != UF2_MAGIC_START0 || uf2->magicStart1 != UF2_MAGIC_START1 || uf2->magicEnd != UF2_MAGIC_END)
        return (int32_t)bufsize;
        
    if (uf2->blockNo == 0) {
        global_state.fw.checksum = 0xffffffff;
    
        /* Make sure nobody else touches the flash during this operation, otherwise we get empty pages */
        global_state.fw.upgrade_in_progress = true;
    }    

    /* Update checksum continuously as blocks are being received */
    const uint32_t last_block_with_checksum = (STAGING_IMAGE_SIZE - FLASH_SECTOR_SIZE) / FLASH_PAGE_SIZE;
    for (int i=0; i<FLASH_PAGE_SIZE && uf2->blockNo < last_block_with_checksum; i++)
        global_state.fw.checksum = crc32_iter(global_state.fw.checksum, buffer[32 + i]);    

    write_flash_page(flash_addr, &buffer[32]);
    
    if (is_final_block) {        
        global_state.fw.checksum = ~global_state.fw.checksum;

        /* If checksums don't match, overwrite first sector and rely on ROM bootloader for recovery */
        if (global_state.fw.checksum != calculate_firmware_crc32()) {
            flash_range_erase((uint32_t)ADDR_FW_RUNNING - XIP_BASE, FLASH_SECTOR_SIZE);
            reset_usb_boot(1 << PICO_DEFAULT_LED_PIN, 0);
        }
        else {
            global_state.reboot_requested = true;
        }
    }    

    /* Provide some visual indication that fw is being uploaded */
    toggle_led();    
    watchdog_update();

    return (int32_t)bufsize;
}

/* This is a super-dumb, rudimentary disk, any other scsi command is simply rejected */
int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void *buffer, uint16_t bufsize) {   
    tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
    return -1;
}
 