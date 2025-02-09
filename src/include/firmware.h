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

 /*==============================================================================
  *  Firmware Update Functions
  *  Functions for managing firmware updates, CRC calculation, and related tasks.
  *==============================================================================*/

 uint32_t calculate_firmware_crc32(void);
 void     reboot(void);
 void     write_flash_page(uint32_t, uint8_t *);

 /*==============================================================================
  *  UART Packet Fetching
  *  Functions to handle incoming UART packets, especially for firmware updates.
  *==============================================================================*/
 void     fetch_packet(device_t *);
 uint32_t get_ptr_delta(uint32_t, device_t *);
 bool     is_start_of_packet(device_t *);
 void     request_byte(device_t *, uint32_t);

 /*==============================================================================
  *  Button Interaction
  *  Functions interacting with the button, e.g. checking if pressed.
  *==============================================================================*/

 bool is_bootsel_pressed(void);
