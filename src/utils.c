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

/**================================================== *
 * ==============  Checksum Functions  ============== *
 * ================================================== */

uint8_t calc_checksum(const uint8_t* data, int length) {
    uint8_t checksum = 0;

    for (int i = 0; i < length; i++) {
        checksum ^= data[i];
    }

    return checksum;
}

bool verify_checksum(const uart_packet_t* packet) {
    uint8_t checksum = calc_checksum(packet->data, PACKET_DATA_LENGTH);
    return checksum == packet->checksum;
}

/**================================================== *
 * ==============  Watchdog Functions  ============== *
 * ================================================== */

void kick_watchdog(void) {
    /* Read the timer AFTER duplicating the core1 timestamp,
       so it doesn't get updated in the meantime. */

    uint64_t core1_last_loop_pass = global_state.core1_last_loop_pass;
    uint64_t current_time = time_us_64();

    /* If core1 stops updating the timestamp, we'll stop kicking the watchog and reboot */
    if (current_time - core1_last_loop_pass < CORE1_HANG_TIMEOUT_US)
        watchdog_update();
}

/* ================================================== *
 * Flash and config functions
 * ================================================== */

void wipe_config(void) {
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE);
    restore_interrupts(ints);
}

void load_config(void) {
    config_t* config = ADDR_CONFIG_BASE_ADDR;

    /* If no config is detected, copy default values to our struct. TODO checksum */
    if (config->magic_header != 0x0B00B1E5) {
        config = (config_t*)&default_config;
    }

    memcpy(&global_state.config, config, sizeof(config_t));
}

void save_config(void) {
    uint8_t buf[FLASH_PAGE_SIZE];
    memcpy(buf, &global_state.config, sizeof(config_t));

    wipe_config();

    uint32_t ints = save_and_disable_interrupts();
    flash_range_program(PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE, buf, FLASH_PAGE_SIZE);
    restore_interrupts(ints);
}