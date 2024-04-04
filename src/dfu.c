/*
 * This file is part of DeskHop (https://github.com/hrvach/deskhop).
 * Copyright (c) 2024 Hrvoje Cavrak
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
#include "usb_descriptors.h"

extern const uint8_t __PROGRAM_ORIGIN;
extern const uint8_t __PROGRAM_END;

extern const uint8_t __FLASH_START;

extern const uint8_t __DFU_BUFFER_ORIGIN;

extern const config_t __CONFIG_ORIGIN;

static const uint8_t *dfu_buffer = &__DFU_BUFFER_ORIGIN;
static size_t dfu_buffer_contents_size;

static uint8_t flash_buf[FLASH_SECTOR_SIZE];

//--------------------------------------------------------------------+
// Device Descriptor
//--------------------------------------------------------------------+

tusb_desc_device_t const desc_device_dfu = {
    .bLength         = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB          = 0x0200,
    .bDeviceClass    = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

    // https://github.com/raspberrypi/usb-pid
    .idVendor  = 0x2E8A,
    .idProduct = 0x107C,   // TODO: will require a second PID for DFU mode
    .bcdDevice = 0x0100,

    .iManufacturer = 0x01,
    .iProduct      = 0x02,
    .iSerialNumber = 0x03,

    .bNumConfigurations = 0x01};

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

enum alt_num_e {
    ALT_BOARD_A_FW = 0,
    ALT_BOARD_B_FW,
    ALT_CONFIG,
    ALT_DFU_BUFFER,
};

#ifdef DFU_DEBUG
#define ALT_COUNT 4
#else
#define ALT_COUNT 3
#endif

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_DFU_DESC_LEN(ALT_COUNT))

uint8_t const desc_configuration_dfu[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1,
			  1,
			  0,
			  CONFIG_TOTAL_LEN,
			  TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP,
			  500),

    // Interface number, Alternate count, starting string index, attributes, detach timeout, transfer size
    TUD_DFU_DESCRIPTOR(0,
		       ALT_COUNT,
		       STRID_DFU_BOARD_A_FW,
                       DFU_ATTR_CAN_DOWNLOAD | DFU_ATTR_CAN_UPLOAD | DFU_ATTR_WILL_DETACH,
		       DFU_MAX_WAIT,
		       CFG_TUD_DFU_XFER_BUFSIZE),
};

//--------------------------------------------------------------------+
// Callbacks
//--------------------------------------------------------------------+

int64_t dfu_timeout_alarm_cb(alarm_id_t id, void *user_data) {
    watchdog_reboot(0x00000000, 0x00000000, 50);

    /* Wait for the reset */
    while(true)
        tight_loop_contents();
}

/* Invoked on DFU_DETACH request to reboot to the bootloader */
void tud_dfu_runtime_reboot_to_dfu_cb(void)
{
    /* Indicate that the next boot should be into DFU mode */
    watchdog_hw->scratch[0] = DFU_BOOT_MODE;
    watchdog_reboot(0x00000000, 0x00000000, 50);

    /* Wait for the reset */
    while(true)
        tight_loop_contents();
}

uint32_t tud_dfu_get_timeout_cb(uint8_t alt, uint8_t state)
{
   if (state == DFU_DNBUSY) {
       /* 100ms for a 'sector erase' plus a 'sector program' */
       return 100;
   }

   /* state == DFU_MANIFEST) */
   /* 100ms per sector to be programmed */
   return 100 * (1 + (dfu_buffer_contents_size / FLASH_SECTOR_SIZE));
}

void tud_dfu_download_cb(uint8_t alt, uint16_t block_num, uint8_t const* data, uint16_t length)
{
    const uint32_t flash_address = &__DFU_BUFFER_ORIGIN - &__FLASH_START + (block_num * FLASH_SECTOR_SIZE);

    flash_range_erase(flash_address, FLASH_SECTOR_SIZE);

    if (length == FLASH_SECTOR_SIZE) {
	flash_range_program(flash_address, data, FLASH_SECTOR_SIZE);
    } else {
	memcpy(flash_buf, data, length);
	flash_range_program(flash_address, flash_buf, FLASH_SECTOR_SIZE);
    }

    tud_dfu_finish_flashing(DFU_STATUS_OK);

    dfu_buffer_contents_size += length;
}

static const uint8_t *download_destination_address = NULL;
static size_t download_bytes_left;

void prepare_firmware_download()
{
    download_destination_address = &__PROGRAM_ORIGIN;
    download_bytes_left = dfu_buffer_contents_size;
}

void prepare_config_download()
{
    download_destination_address = (const uint8_t *) &__CONFIG_ORIGIN;
    download_bytes_left = dfu_buffer_contents_size;
}

void local_manifest_cb()
{
    const uint8_t *source_address = dfu_buffer;
    uint32_t flash_address = download_destination_address - &__FLASH_START;

    while (download_bytes_left > 0) {
	flash_range_erase(flash_address, FLASH_SECTOR_SIZE);

	uint32_t bytes_to_copy = download_bytes_left >= FLASH_SECTOR_SIZE ? FLASH_SECTOR_SIZE : download_bytes_left;

	memcpy(flash_buf, source_address, bytes_to_copy);
	download_bytes_left -= bytes_to_copy;

	flash_range_program(flash_address, flash_buf, FLASH_SECTOR_SIZE);
	source_address += FLASH_SECTOR_SIZE;
	flash_address += FLASH_SECTOR_SIZE;
    }
}

void tud_dfu_manifest_cb(uint8_t alt)
{
    switch (alt) {
    case ALT_BOARD_A_FW:
	if (BOARD_ROLE == PICO_A) {
	    prepare_firmware_download();
	    local_manifest_cb();
	}
    case ALT_BOARD_B_FW:
	if (BOARD_ROLE == PICO_B) {
	    prepare_firmware_download();
	    local_manifest_cb();
	}
    case ALT_CONFIG:
	prepare_firmware_download();
	local_manifest_cb();
    }

    tud_dfu_finish_flashing(DFU_STATUS_OK);

#ifndef DFU_DEBUG
    /* the manifest is complete, reboot in 1 second to
       ensure that any remaining USB communications
       have time to be completed */
    add_alarm_in_ms(1000, dfu_timeout_alarm_cb, NULL, false);
#endif
}

static const uint8_t *upload_source_address = NULL;
static size_t upload_bytes_left;

void prepare_firmware_upload()
{
    upload_source_address = &__PROGRAM_ORIGIN;
    upload_bytes_left = &__PROGRAM_END - &__PROGRAM_ORIGIN;
}

void prepare_config_upload()
{
    upload_source_address = (uint8_t *) &global_state.config;
    upload_bytes_left = sizeof(global_state.config);
}

void prepare_dfu_buffer_upload()
{
    upload_source_address = dfu_buffer;
    upload_bytes_left = dfu_buffer_contents_size;
}

uint16_t local_upload_cb(uint8_t* data, uint16_t length)
{
    uint16_t bytes_to_send = (upload_bytes_left > length) ? length : upload_bytes_left;

    memcpy(data, upload_source_address, bytes_to_send);

    upload_source_address += bytes_to_send;
    upload_bytes_left -= bytes_to_send;
    if (upload_bytes_left == 0) {
	upload_source_address = NULL;

	/* the upload is complete, reboot in 1 second to
	   ensure that any remaining USB communications
	   have time to be completed */
	add_alarm_in_ms(1000, dfu_timeout_alarm_cb, NULL, false);
    }

    return bytes_to_send;
}

uint16_t tud_dfu_upload_cb(uint8_t alt, uint16_t block_num, uint8_t* data, uint16_t length)
{
    switch (alt) {
    case ALT_BOARD_A_FW:
	if (BOARD_ROLE == PICO_A) {
	    if (upload_source_address == NULL) {
		prepare_firmware_upload();
	    }
	    return local_upload_cb(data, length);
	} else {
	    return 0;
	}
    case ALT_BOARD_B_FW:
	if (BOARD_ROLE == PICO_B) {
	    if (upload_source_address == NULL) {
		prepare_firmware_upload();
	    }
	    return local_upload_cb(data, length);
	} else {
	    return 0;
	}
    case ALT_CONFIG:
	if (upload_source_address == NULL) {
	    prepare_config_upload();
	}
	return local_upload_cb(data, length);
    case ALT_DFU_BUFFER:
	if (upload_source_address == NULL) {
	    prepare_dfu_buffer_upload();
	}
	return local_upload_cb(data, length);
    }

    return 0;
}

// Invoked when the Host has terminated a download or upload transfer
void tud_dfu_abort_cb(uint8_t alt)
{
    switch (alt) {
    case ALT_BOARD_A_FW:
	if (BOARD_ROLE == PICO_A) {
	    upload_source_address = NULL;
	}
	dfu_buffer_contents_size = 0;
    case ALT_BOARD_B_FW:
	if (BOARD_ROLE == PICO_B) {
	    upload_source_address = NULL;
	}
	dfu_buffer_contents_size = 0;
    case ALT_CONFIG:
	upload_source_address = NULL;
    case ALT_DFU_BUFFER:
	upload_source_address = NULL;
    }

    /* restart the maximum wait timer for the next
       upload or download */
    cancel_alarm(global_state.dfu_timeout_alarm);
    global_state.dfu_timeout_alarm = add_alarm_in_ms(DFU_MAX_WAIT, dfu_timeout_alarm_cb, NULL, false);
}
