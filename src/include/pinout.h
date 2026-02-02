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

/*==============================================================================
 *  Board Roles
 *==============================================================================*/

#define BOARD_ROLE (global_state.board_role)
#define OTHER_ROLE (BOARD_ROLE == OUTPUT_A ? OUTPUT_B : OUTPUT_A)

/*==============================================================================
 *  Board Type: Adafruit Feather RP2040 USB Host
 *==============================================================================*/
 
#ifdef ADAFRUIT_FEATHER_RP2040_USB_HOST

/*==============================================================================
 *  GPIO Pins (LED, USB)
 *==============================================================================*/
 
#define GPIO_LED_PIN   13 // LED is connected to pin 13 on a Feather RP2040 USB Host
#define PIO_USB_DP_PIN 16 // D+ is pin 16, D- is pin 17
#define GPIO_USB_PWR_PIN 18

/*==============================================================================
 *  Serial Pins
 *==============================================================================*/

/* GP0 / GP1, Pins TX (TX), RX (RX) on the Feather board */
#define BOARD_A_RX 01
#define BOARD_A_TX 00

/* GP28 / GP29, Pins A2 (TX), A3 (RX) on the Feather board */
#define BOARD_B_RX 29
#define BOARD_B_TX 28

/*==============================================================================
 *  Board Type: PICO Series and pin compatible
 *==============================================================================*/

#else

/*==============================================================================
 *  GPIO Pins (LED, USB)
 *==============================================================================*/

#define GPIO_LED_PIN   25 // LED is connected to pin 25 on a PICO
#define PIO_USB_DP_PIN 14 // D+ is pin 14, D- is pin 15

/*==============================================================================
 *  Board Type: Adafruit Feather RP2040 USB Host
 *==============================================================================*/

/* GP12 / GP13, Pins 16 (TX), 17 (RX) on the Pico board */
#define BOARD_A_RX 13
#define BOARD_A_TX 12

/* GP16 / GP17, Pins 21 (TX), 22 (RX) on the Pico board */
#define BOARD_B_RX 17
#define BOARD_B_TX 16

#endif

#define SERIAL_RX_PIN (global_state.board_role == OUTPUT_A ? BOARD_A_RX : BOARD_B_RX)
#define SERIAL_TX_PIN (global_state.board_role == OUTPUT_A ? BOARD_A_TX : BOARD_B_TX)
