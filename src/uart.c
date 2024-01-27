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
 * ===============  Sending Packets  ================ *
 * ================================================== */

void send_packet(const uint8_t *data, enum packet_type_e packet_type, int length) {
    uint8_t raw_packet[RAW_PACKET_LENGTH] = {[0] = START1,
                                             [1] = START2,
                                             [2] = packet_type,
                                             /* [3-10] is data, defaults to 0 */
                                             [11] = calc_checksum(data, length)};

    if (length > 0)
        memcpy(&raw_packet[START_LENGTH + TYPE_LENGTH], data, length);

    /* Packets are short, fixed length, high speed and there is no flow control to block this */
    uart_write_blocking(SERIAL_UART, raw_packet, RAW_PACKET_LENGTH);
}

void send_value(const uint8_t value, enum packet_type_e packet_type) {
    const uint8_t data = value;
    send_packet(&data, packet_type, sizeof(uint8_t));
}

/**================================================== *
 * ===============  Parsing Packets  ================ *
 * ================================================== */

const uart_handler_t uart_handler[] = {
    {.type = KEYBOARD_REPORT_MSG, .handler = handle_keyboard_uart_msg},
    {.type = MOUSE_REPORT_MSG, .handler = handle_mouse_abs_uart_msg},
    {.type = OUTPUT_SELECT_MSG, .handler = handle_output_select_msg},
    {.type = FIRMWARE_UPGRADE_MSG, .handler = handle_fw_upgrade_msg},
    {.type = MOUSE_ZOOM_MSG, .handler = handle_mouse_zoom_msg},
    {.type = KBD_SET_REPORT_MSG, .handler = handle_set_report_msg},
    {.type = SWITCH_LOCK_MSG, .handler = handle_switch_lock_msg},
    {.type = SYNC_BORDERS_MSG, .handler = handle_sync_borders_msg},
    {.type = FLASH_LED_MSG, .handler = handle_flash_led_msg},
    {.type = SCREENSAVER_MSG, .handler = handle_screensaver_msg},
    {.type = WIPE_CONFIG_MSG, .handler = handle_wipe_config_msg},
};

void process_packet(uart_packet_t *packet, device_t *state) {
    if (!verify_checksum(packet))
        return;

    for (int i = 0; i < ARRAY_SIZE(uart_handler); i++) {
        if (uart_handler[i].type == packet->type) {
            uart_handler[i].handler(packet, state);
            return;
        }
    }
}

/**================================================== *
 * ==============  Receiving Packets  =============== *
 * ================================================== */

/* We are in IDLE state until we detect the packet start (0xAA 0x55) */
void handle_idle_state(uint8_t *raw_packet, device_t *state) {
    if (!uart_is_readable(SERIAL_UART)) {
        return;
    }

    raw_packet[0] = raw_packet[1];          /* Remember the previous byte received */
    raw_packet[1] = uart_getc(SERIAL_UART); /* Try to match packet start */

    /* If we found 0xAA 0x55, we're in sync and can move on to read/process the packet */
    if (raw_packet[0] == START1 && raw_packet[1] == START2) {
        state->receiver_state = READING_PACKET;
    }
}

/* Read a character off the line until we reach fixed packet length */
void handle_reading_state(uint8_t *raw_packet, device_t *state, int *count) {
    while (uart_is_readable(SERIAL_UART) && *count < PACKET_LENGTH) {        
        /* Read and store the incoming byte */
        raw_packet[(*count)++] = uart_getc(SERIAL_UART);
    }

    /* Check if a complete packet is received */
    if (*count >= PACKET_LENGTH) {
        state->receiver_state = PROCESSING_PACKET;
    }
}

/* Process that packet, restart counters and state machine to have it back to IDLE */
void handle_processing_state(uart_packet_t *packet, device_t *state, int *count) {
    process_packet(packet, state);
    state->receiver_state = IDLE;
    *count                = 0;
}

/* Very simple state machine to receive and process packets over serial */
void receive_char(uart_packet_t *packet, device_t *state) {
    uint8_t *raw_packet = (uint8_t *)packet;
    static int count    = 0;

    switch (state->receiver_state) {
        case IDLE:
            handle_idle_state(raw_packet, state);
            break;

        case READING_PACKET:
            handle_reading_state(raw_packet, state, &count);
            break;

        case PROCESSING_PACKET:
            handle_processing_state(packet, state, &count);
            break;
    }
}
