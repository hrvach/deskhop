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

uint8_t calc_checksum(const uint8_t *data, int length) {
    uint8_t checksum = 0;

    for (int i = 0; i < length; i++) {
        checksum ^= data[i];
    }

    return checksum;
}

bool verify_checksum(const uart_packet_t *packet) {
    uint8_t checksum = calc_checksum(packet->data, PACKET_DATA_LENGTH);
    return checksum == packet->checksum;
}

/**================================================== *
 * ==============  Watchdog Functions  ============== *
 * ================================================== */

void kick_watchdog(device_t *state) {
    /* Read the timer AFTER duplicating the core1 timestamp,
       so it doesn't get updated in the meantime. */

    uint64_t core1_last_loop_pass = state->core1_last_loop_pass;
    uint64_t current_time         = time_us_64();

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

void load_config(device_t *state) {
    const config_t *config   = ADDR_CONFIG_BASE_ADDR;
    config_t *running_config = &state->config;

    /* Load the flash config first, including the checksum */
    memcpy(running_config, config, sizeof(config_t));

    /* Calculate and update checksum, size without checksum */
    uint8_t checksum = calc_checksum((uint8_t *)running_config, sizeof(config_t) - sizeof(uint32_t));

    /* We expect a certain byte to start the config header */
    bool magic_header_fail = (running_config->magic_header != 0xB00B1E5);

    /* We expect the checksum to match */
    bool checksum_fail = (running_config->checksum != checksum);

    /* We expect the config version to match exactly, to avoid erroneous values */
    bool version_fail = (running_config->version != CURRENT_CONFIG_VERSION);

    /* On any condition failing, we fall back to default config */
    if (magic_header_fail || checksum_fail || version_fail)
        memcpy(running_config, &default_config, sizeof(config_t));
}

void save_config(device_t *state) {
    uint8_t buf[FLASH_PAGE_SIZE];
    uint8_t *raw_config = (uint8_t *)&state->config;

    /* Calculate and update checksum, size without checksum */
    uint8_t checksum       = calc_checksum(raw_config, sizeof(config_t) - sizeof(uint32_t));
    state->config.checksum = checksum;

    /* Copy the config to buffer and wipe the old one */
    memcpy(buf, raw_config, sizeof(config_t));
    wipe_config();

    /* Disable interrupts, then write the flash page and re-enable */
    uint32_t ints = save_and_disable_interrupts();
    flash_range_program(PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE, buf, FLASH_PAGE_SIZE);
    restore_interrupts(ints);
}

/* Have something fun and entertaining when idle */
void screensaver_task(device_t *state) {
    const int mouse_move_delay     = 5000;

    static mouse_abs_report_t report = {.x = 0, .y = 0};
    static int last_pointer_move     = 0;

    uint64_t current_time = time_us_64();
    static uint64_t last_activation_time = 0;

    /* "Randomly" chosen initial values */
    static int dx = 20;
    static int dy = 25;

    /* If the maximum time has been reached, nothing to do here. */
    if (state->screensaver_max_time_reached[BOARD_ROLE]) {
	return;
    }

    /* If we're not enabled, nothing to do here. */
    if (!state->config.screensaver[BOARD_ROLE].enabled) {
	last_activation_time = 0;
	return;
    }

    /* If we're not the selected output and that is required, nothing to do here. */
    if (state->config.screensaver[BOARD_ROLE].only_if_inactive &&
	CURRENT_BOARD_IS_ACTIVE_OUTPUT) {
	last_activation_time = 0;
        return;
    }

    /* We are enabled, but idle time still too small to activate. */
    if ((current_time - state->last_activity[BOARD_ROLE]) <
	state->config.screensaver[BOARD_ROLE].idle_time_us) {
	last_activation_time = 0;
        return;
    }

    if (last_activation_time == 0) {
	last_activation_time = current_time;
    } else {
	/* We are enabled, but max time has been reached. */
	if ((current_time - last_activation_time) >
	    state->config.screensaver[BOARD_ROLE].max_time_us) {
	    state->screensaver_max_time_reached[BOARD_ROLE] = true;
	    last_activation_time = 0;
	    return;
	}
    }

    /* We're active! Now check if it's time to move the cursor yet. */
    if ((time_us_32()) - last_pointer_move < mouse_move_delay)
        return;

    /* Check if we are bouncing off the walls and reverse direction in that case. */
    if (report.x + dx < MIN_SCREEN_COORD || report.x + dx > MAX_SCREEN_COORD)
        dx = -dx;

    if (report.y + dy < MIN_SCREEN_COORD || report.y + dy > MAX_SCREEN_COORD)
        dy = -dy;

    report.x += dx;
    report.y += dy;

    /* Move mouse pointer */
    queue_mouse_report(&report, state);

    /* Update timer of the last pointer move */
    last_pointer_move = time_us_32();
}
