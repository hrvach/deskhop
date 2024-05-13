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
 * ===========  TinyUSB Device Callbacks  =========== *
 * ================================================== */

/* Invoked when we get GET_REPORT control request.
 * We are expected to fill buffer with the report content, update reqlen
 * and return its length. We return 0 to STALL the request. */
uint16_t tud_hid_get_report_cb(uint8_t instance,
                               uint8_t report_id,
                               hid_report_type_t report_type,
                               uint8_t *buffer,
                               uint16_t request_len) {
    return 0;
}

/**
 * Computer controls our LEDs by sending USB SetReport messages with a payload
 * of just 1 byte and report type output. It's type 0x21 (USB_REQ_DIR_OUT |
 * USB_REQ_TYP_CLASS | USB_REQ_REC_IFACE) Request code for SetReport is 0x09,
 * report type is 0x02 (HID_REPORT_TYPE_OUTPUT). We get a set_report callback
 * from TinyUSB device HID and then figure out what to do with the LEDs.
 */
void tud_hid_set_report_cb(uint8_t instance,
                           uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const *buffer,
                           uint16_t bufsize) {
    if (report_id != REPORT_ID_KEYBOARD || bufsize != 1 || report_type != HID_REPORT_TYPE_OUTPUT)
        return;

    uint8_t leds = buffer[0];

    /* If we are using caps lock LED to indicate the chosen output, that has priority */
    if (KBD_LED_AS_INDICATOR) {
        leds = leds & 0xFD; /* 1111 1101 (Clear Caps Lock bit) */

        if (global_state.active_output)
            leds |= KEYBOARD_LED_CAPSLOCK;
    }

    global_state.keyboard_leds[global_state.active_output] = leds;

    /* If the board doesn't have the keyboard hooked up directly, we need to relay this information
       to the other one that has (and LEDs you can turn on). */
    if (global_state.keyboard_connected)
        restore_leds(&global_state);
    else
        send_value(leds, KBD_SET_REPORT_MSG);
}

/* Invoked when device is mounted */
void tud_mount_cb(void) {
    global_state.tud_connected = true;
}

/* Invoked when device is unmounted */
void tud_umount_cb(void) {
    global_state.tud_connected = false;
}

/**================================================== *
 * ===============  USB HOST Section  =============== *
 * ================================================== */

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

    switch (itf_protocol) {
        case HID_ITF_PROTOCOL_KEYBOARD:
            global_state.keyboard_connected = false;
            memset(&global_state.kbd_dev, 0, sizeof(global_state.kbd_dev));
            break;

        case HID_ITF_PROTOCOL_MOUSE:
            global_state.mouse_connected = false;

            /* Clear this so reconnecting a mouse doesn't try to continue in HID REPORT protocol */
            memset(&global_state.mouse_dev, 0, sizeof(global_state.mouse_dev));
            break;
    }
}

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len) {
    uint8_t const itf_protocol                          = tuh_hid_interface_protocol(dev_addr, instance);
    tuh_hid_report_info_t report_info[MAX_REPORT_ITEMS] = {0};
    tuh_hid_report_info_t *info;

    switch (itf_protocol) {
        case HID_ITF_PROTOCOL_KEYBOARD:
            if (ENFORCE_PORTS && BOARD_ROLE == PICO_B)
                return;

            /* Keeping this is required for setting leds from device set_report callback */
            global_state.kbd_dev_addr       = dev_addr;
            global_state.kbd_instance       = instance;
            global_state.keyboard_connected = true;
            break;

        case HID_ITF_PROTOCOL_MOUSE:
            if (ENFORCE_PORTS && BOARD_ROLE == PICO_A)
                return;

            /* Switch to using protocol report instead of boot report, it's more complicated but
               at least we get all the information we need (looking at you, mouse wheel) */
            if (tuh_hid_get_protocol(dev_addr, instance) == HID_PROTOCOL_BOOT) {
                tuh_hid_set_protocol(dev_addr, instance, HID_PROTOCOL_REPORT);
            }

            global_state.mouse_dev.protocol = tuh_hid_get_protocol(dev_addr, instance);
            parse_report_descriptor(&global_state.mouse_dev, MAX_REPORTS, desc_report, desc_len);

            global_state.mouse_connected = true;
            break;

        case HID_ITF_PROTOCOL_NONE:
            uint8_t num_parsed
                = tuh_hid_parse_report_descriptor(&report_info[0], MAX_REPORT_ITEMS, desc_report, desc_len);

            for (int report_num = 0; report_num < num_parsed; report_num++) {
                info = &report_info[report_num];

                if (info->usage == HID_USAGE_CONSUMER_CONTROL && info->usage_page == HID_USAGE_PAGE_CONSUMER)
                    global_state.kbd_dev.consumer_report_id = info->report_id;
            }
            break;
    }
    /* Flash local led to indicate a device was connected */
    blink_led(&global_state);

    /* Also signal the other board to flash LED, to enable easy verification if serial works */
    send_value(ENABLE, FLASH_LED_MSG);

    /* Kick off the report querying */
    tuh_hid_receive_report(dev_addr, instance);
}

/* Invoked when received report from device via interrupt endpoint */
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len) {
    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

    switch (itf_protocol) {
        case HID_ITF_PROTOCOL_KEYBOARD:
            process_keyboard_report((uint8_t *)report, len, &global_state);
            break;

        case HID_ITF_PROTOCOL_MOUSE:
            process_mouse_report((uint8_t *)report, len, &global_state);
            break;

        case HID_ITF_PROTOCOL_NONE:
            process_consumer_report((uint8_t *)report, len, &global_state);            
            break;
    }

    /* Continue requesting reports */
    tuh_hid_receive_report(dev_addr, instance);
}

/* Set protocol in a callback. If we were called, command succeeded. We're only
   doing this for the mouse for now, so we can only be called about the mouse */
void tuh_hid_set_protocol_complete_cb(uint8_t dev_addr, uint8_t idx, uint8_t protocol) {
    global_state.mouse_dev.protocol = protocol;
}
