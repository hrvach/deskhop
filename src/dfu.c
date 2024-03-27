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

#define ALT_COUNT 3

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
	   return 100;
   }
   else if (state == DFU_MANIFEST) {
	   return 0;
   }

   return 0;
}

void tud_dfu_download_cb(uint8_t alt, uint16_t block_num, uint8_t const* data, uint16_t length)
{
  tud_dfu_finish_flashing(DFU_STATUS_OK);
}

void tud_dfu_manifest_cb(uint8_t alt)
{
  tud_dfu_finish_flashing(DFU_STATUS_OK);
}

uint16_t tud_dfu_upload_cb(uint8_t alt, uint16_t block_num, uint8_t* data, uint16_t length)
{
  return 0;
}

// Invoked when the Host has terminated a download or upload transfer
void tud_dfu_abort_cb(uint8_t alt)
{
}
