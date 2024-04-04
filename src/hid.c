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

tusb_desc_device_t const desc_device_hid = {
	.bLength         = sizeof(tusb_desc_device_t),
	.bDescriptorType = TUSB_DESC_DEVICE,
	.bcdUSB          = 0x0200,
	.bDeviceClass    = 0x00,
	.bDeviceSubClass = 0x00,
	.bDeviceProtocol = 0x00,
	.bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

	// https://github.com/raspberrypi/usb-pid
	.idVendor  = 0x2E8A,
	.idProduct = 0x107C,
	.bcdDevice = 0x0100,

	.iManufacturer = 0x01,
	.iProduct      = 0x02,
	.iSerialNumber = 0x03,

	.bNumConfigurations = 0x01};

//--------------------------------------------------------------------+
// HID Report Descriptor
//--------------------------------------------------------------------+

enum itf_num_e {
	ITF_NUM_HID,
	ITF_NUM_HID_REL_M,
#if DFU_RT_ENABLED
	ITF_NUM_DFU_RT,
#endif
#ifdef DH_DEBUG
	ITF_NUM_CDC,
#endif
	ITF_NUM_TOTAL
};

// Relative mouse is used to overcome limitations of multiple desktops on MacOS and Windows
uint8_t const desc_hid_report[] = {TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(REPORT_ID_KEYBOARD)),
                                   TUD_HID_REPORT_DESC_ABSMOUSE(HID_REPORT_ID(REPORT_ID_MOUSE))};

uint8_t const desc_hid_report_relmouse[] = {TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(REPORT_ID_RELMOUSE))};

// Invoked when received GET HID REPORT DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
    if (instance == ITF_NUM_HID_REL_M) {
        return desc_hid_report_relmouse;
    }

    /* Default */
    return desc_hid_report;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

#define EPNUM_HID       0x81
#define EPNUM_HID_REL_M 0x82

#ifndef DH_DEBUG

#if DFU_RT_ENABLED
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN + TUD_HID_DESC_LEN + TUD_DFU_RT_DESC_LEN)
#else
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN + TUD_HID_DESC_LEN)
#endif

#else

#if DFU_RT_ENABLED
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN + TUD_HID_DESC_LEN + TUD_DFU_RT_DESC_LEN + TUD_CDC_DESC_LEN)
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN + TUD_HID_DESC_LEN + TUD_CDC_DESC_LEN)
#else
#endif

#define EPNUM_CDC_NOTIF  0x83
#define EPNUM_CDC_OUT    0x04
#define EPNUM_CDC_IN     0x84

#endif

uint8_t const desc_configuration_hid[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1,
			  ITF_NUM_TOTAL,
			  0,
			  CONFIG_TOTAL_LEN,
			  TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP,
			  500),

    // Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval
    TUD_HID_DESCRIPTOR(ITF_NUM_HID,
                       STRID_PRODUCT,
                       HID_ITF_PROTOCOL_NONE,
                       sizeof(desc_hid_report),
                       EPNUM_HID,
                       CFG_TUD_HID_EP_BUFSIZE,
                       1),

    TUD_HID_DESCRIPTOR(ITF_NUM_HID_REL_M,
                       STRID_MOUSE,
                       HID_ITF_PROTOCOL_NONE,
                       sizeof(desc_hid_report_relmouse),
                       EPNUM_HID_REL_M,
                       CFG_TUD_HID_EP_BUFSIZE,
                       1),

#if DFU_RT_ENABLED
    // Interface number, string index, attributes, detach timeout, transfer size */
    TUD_DFU_RT_DESCRIPTOR(ITF_NUM_DFU_RT,
			  STRID_DFU_RUNTIME,
			  DFU_ATTR_CAN_DOWNLOAD | DFU_ATTR_CAN_UPLOAD | DFU_ATTR_WILL_DETACH,
			  DFU_MAX_WAIT,
			  CFG_TUD_DFU_XFER_BUFSIZE),
#endif

#ifdef DH_DEBUG
    // Interface number, string index, EP notification address and size, EP data address (out, in) and size.
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC,
		       STRID_DEBUG,
		       EPNUM_CDC_NOTIF,
		       8,
		       EPNUM_CDC_OUT,
		       EPNUM_CDC_IN,
		       CFG_TUD_CDC_EP_BUFSIZE),
#endif
};

//--------------------------------------------------------------------+
// Callbacks
//--------------------------------------------------------------------+

//--------------------------------------------------------------------+
// Helpers
//--------------------------------------------------------------------+

bool tud_mouse_report(uint8_t mode,
                      uint8_t buttons,
                      int16_t x,
                      int16_t y,
                      int8_t wheel) {

    if (mode == ABSOLUTE) {
        mouse_report_t report = {.buttons = buttons, .x = x, .y = y, .wheel = wheel};
        return tud_hid_n_report(ITF_NUM_HID, REPORT_ID_MOUSE, &report, sizeof(report));
    }
    else {
        hid_mouse_report_t report = {.buttons = buttons, .x = x - 16384, .y = y - 16384, .wheel = wheel, .pan = 0};
        return tud_hid_n_report(ITF_NUM_HID_REL_M, REPORT_ID_RELMOUSE, &report, sizeof(report));
    }
}
