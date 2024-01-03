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

void send_packet(const uint8_t* data, enum packet_type_e packet_type, int length) {
    uint8_t raw_packet[RAW_PACKET_LENGTH] = {[0] = START1,
                                             [1] = START2,
                                             [2] = packet_type,
                                             /* [3-10] is data, defaults to 0 */
                                             [11] = calc_checksum(data, length)};

    if (length > 0)
        memcpy(&raw_packet[START_LENGTH + TYPE_LENGTH], data, length);

    uart_write_blocking(SERIAL_UART, raw_packet, RAW_PACKET_LENGTH);
}

void send_value(const uint8_t value, enum packet_type_e packet_type) {
    const uint8_t data[8] = {[0] = value};

    send_packet((uint8_t*)&data, packet_type, 1);
}

/**================================================== *
 * ===============  Parsing Packets  ================ *
 * ================================================== */


void process_packet(uart_packet_t* packet, device_state_t* state) {
    if (!verify_checksum(packet)) {
        return;
    }

    switch (packet->type) {
        case KEYBOARD_REPORT_MSG:
            handle_keyboard_uart_msg(packet, state);
            break;

        case MOUSE_REPORT_MSG:
            handle_mouse_abs_uart_msg(packet, state);
            break;

        case OUTPUT_SELECT_MSG:
            handle_output_select_msg(packet, state);
            break;

        case FIRMWARE_UPGRADE_MSG:
            handle_fw_upgrade_msg();
            break;

        case MOUSE_ZOOM_MSG:
            handle_mouse_zoom_msg(packet, state);
            break;

        case KBD_SET_REPORT_MSG:
            handle_set_report_msg(packet, state);
            break;

        case SWITCH_LOCK_MSG:
            handle_switch_lock_msg(packet, state);
            break;
    }
}

/**================================================== *
 * ==============  Receiving Packets  =============== *
 * ================================================== */

void receive_char(uart_packet_t* packet, device_state_t* state) {
    uint8_t* raw_packet = (uint8_t*)packet;
    static int count = 0;

    switch (state->receiver_state) {
        case IDLE:
            if (uart_is_readable(SERIAL_UART)) {
                raw_packet[0] = raw_packet[1];           /* Remember the previous byte received */
                raw_packet[1] = uart_getc(SERIAL_UART);  /* ... and try to match packet start */

                /* If we found 0xAA 0x55, we're in sync and can move on to read/process the packet */
                if (raw_packet[0] == START1 && raw_packet[1] == START2) {
                    state->receiver_state = READING_PACKET;
                }
            }
            break;

        case READING_PACKET:
            if (uart_is_readable(SERIAL_UART)) {
                raw_packet[count++] = uart_getc(SERIAL_UART);

                /* Check if a complete packet is received */
                if (count >= PACKET_LENGTH) {
                    state->receiver_state = PROCESSING_PACKET;
                }
            }
            break;

        case PROCESSING_PACKET:
            process_packet(packet, state);

            /* Cleanup and return to IDLE when done */
            count = 0;
            state->receiver_state = IDLE;
            break;
    }
}
