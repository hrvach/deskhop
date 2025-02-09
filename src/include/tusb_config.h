/*
 * This file is part of DeskHop (https://github.com/hrvach/deskhop).
 * Copyright (c) 2025 Hrvoje Cavrak
 * Based on the example by Ha Thach
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * See the file LICENSE for the full license text.
 */

#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

/*==============================================================================
 *  Common Configuration
 *  Settings applicable to both device and host modes.
 *==============================================================================*/

#define CFG_TUSB_OS OPT_OS_PICO

/*==============================================================================
 *  Device Mode Configuration
 *  Settings specific to USB device mode operation.
 *==============================================================================*/

// Enable the TinyUSB device stack.
#define CFG_TUD_ENABLED 1

// Root Hub port number used for the device (typically port 0).
#define BOARD_TUD_RHPORT 0

// Device mode configuration: sets the mode (device) and speed.
#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE | BOARD_DEVICE_RHPORT_SPEED)

// Root Hub port maximum operational speed (defaults to Full Speed).
#ifndef BOARD_DEVICE_RHPORT_SPEED
#define BOARD_DEVICE_RHPORT_SPEED OPT_MODE_FULL_SPEED
#endif

// Root Hub port number (defaults to 0).
#ifndef BOARD_DEVICE_RHPORT_NUM
#define BOARD_DEVICE_RHPORT_NUM 0
#endif

/*==============================================================================
 *  Host Mode Configuration
 *  Settings specific to USB host mode operation.
 *==============================================================================*/

// Enable the TinyUSB host stack (requires Pico-PIO-USB).
#define CFG_TUH_ENABLED     1
#define CFG_TUH_RPI_PIO_USB 1

// Root Hub port number used for the host (typically port 1).
#define BOARD_TUH_RHPORT 1
// Host mode configuration: sets the mode (host) and speed
#define CFG_TUSB_RHPORT1_MODE (OPT_MODE_HOST | BOARD_DEVICE_RHPORT_SPEED)

/*==============================================================================
 *  Memory Configuration
 *  Settings for USB memory sections and alignment.
 *==============================================================================*/

// Define a custom memory section for USB buffers (optional).
#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif

// Define the memory alignment for USB buffers (typically 4-byte aligned).
#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))
#endif

/*==============================================================================
 *  Device: Endpoint 0 Size
 *  Configuration for the default control endpoint (Endpoint 0).
 *==============================================================================*/

// Size of the control endpoint (Endpoint 0) buffer.
#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE 64
#endif

/*==============================================================================
 *  Device: CDC Class Configuration (Serial Communication)
 *  Settings for the CDC (Communication Device Class) for serial communication.
 *==============================================================================*/

#ifdef DH_DEBUG

// Enable CDC class for debugging over serial.
#define CFG_TUD_CDC           1

// Use a custom debug printf function.
#define CFG_TUSB_DEBUG_PRINTF dh_debug_printf
extern int dh_debug_printf(const char *__restrict __format, ...);

// Buffer sizes for CDC RX and TX.
#define CFG_TUD_CDC_RX_BUFSIZE 64
#define CFG_TUD_CDC_TX_BUFSIZE 64

// Line coding settings to use on CDC enumeration.
#define CFG_TUH_CDC_LINE_CODING_ON_ENUM \
    { 921600, CDC_LINE_CONDING_STOP_BITS_1, CDC_LINE_CONDING_PARITY_NONE, 8 }

#else
// Disable CDC class when not debugging.
#define CFG_TUD_CDC 0
#endif

/*==============================================================================
 *  Device: Class Configuration
 *  Enable/disable various USB device classes.
 *==============================================================================*/

// Enable HID (Human Interface Device) class (keyboard, mouse, etc.).
#define CFG_TUD_HID    2

// Enable MSC (Mass Storage Class) class.
#define CFG_TUD_MSC    1

/*==============================================================================
 *  Device: Endpoint Buffer Sizes
 *  Configuration for endpoint buffer sizes for different classes.
 *==============================================================================*/

// HID endpoint buffer size (must be large enough for report ID + data).
#define CFG_TUD_HID_EP_BUFSIZE 32

// MSC endpoint buffer size.
#define CFG_TUD_MSC_EP_BUFSIZE 512

/*==============================================================================
 *  Host: Enumeration Buffer Size
 *  Configuration for the buffer used during device enumeration.
 *==============================================================================*/

#define CFG_TUH_ENUMERATION_BUFSIZE 512

/*==============================================================================
 *  Host: Class Configuration
 *  Settings for the USB host class drivers.
 *==============================================================================*/

// Enable USB Hub support.
#define CFG_TUH_HUB 1

// Maximum number of connected devices (excluding the hub itself).
#define CFG_TUH_DEVICE_MAX (CFG_TUH_HUB ? 4 : 1) // Hub typically has 4 ports

// Maximum number of HID instances.
#define CFG_TUH_HID               3 * CFG_TUH_DEVICE_MAX

// HID endpoint buffer sizes (IN and OUT).
#define CFG_TUH_HID_EPIN_BUFSIZE  64
#define CFG_TUH_HID_EPOUT_BUFSIZE 64

#endif /* _TUSB_CONFIG_H_ */
