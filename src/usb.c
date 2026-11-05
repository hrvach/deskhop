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

#include "main.h"
#include "hid_report.h"

_Static_assert(MAX_DEVICES <= CFG_TUH_DEVICE_MAX,
               "MAX_DEVICES must not exceed CFG_TUH_DEVICE_MAX");

/* ================================================== *
 * ===========  TinyUSB Device Callbacks  =========== *
 * ================================================== */

/* Answer GET_REPORT with the current LED state. */
static uint16_t get_led_report(uint8_t *buffer, uint16_t request_len) {
    /* Guardrails for size ... */
    if (request_len < 1)
        return 0;

    /* ... to ensure we don't write to memory outside the provided buffer */
    buffer[0] = global_state.keyboard_leds_desired[BOARD_ROLE];

    /* Return the number of bytes written to the buffer. LED report is 1 byte. */
    return 1;
}

/* Answer GET_REPORT with the current keyboard state. */
static uint16_t get_keyboard_report(uint8_t *buffer, uint16_t request_len) {
    hid_keyboard_report_t report = {0};

    /* If the request buffer is too small, we cannot provide the report */
    if (request_len < sizeof(report))
        return 0;

    /* Inactive output should not be able to read the keyboard state */
    if (CURRENT_BOARD_IS_ACTIVE_OUTPUT)
        combine_kbd_states(&global_state, &report);

    memcpy(buffer, &report, sizeof(report));
    return sizeof(report);
}

/* Return the current keyboard and LED state for GET_REPORT. */
uint16_t tud_hid_get_report_cb(uint8_t instance,
                               uint8_t report_id,
                               hid_report_type_t report_type,
                               uint8_t *buffer,
                               uint16_t request_len) {
    /* Only the keyboard interface answers GET reports. */
    if (instance != ITF_NUM_HID)
        return 0;

    /* Boot protocol omits the report ID, report protocol uses REPORT_ID_KEYBOARD. */
    if (report_id != REPORT_ID_NONE && report_id != REPORT_ID_KEYBOARD)
        return 0;

    switch (report_type) {
        case HID_REPORT_TYPE_OUTPUT:
            return get_led_report(buffer, request_len);

        case HID_REPORT_TYPE_INPUT:
            return get_keyboard_report(buffer, request_len);

        default:
            return 0;
    }
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

    /* We received a report on the config report ID */
    if (instance == ITF_NUM_HID_VENDOR && report_id == REPORT_ID_VENDOR) {
        /* Security - only if config mode is enabled are we allowed to do anything. While the report_id
           isn't even advertised when not in config mode, security must always be explicit and never assume */
        if (!global_state.config_mode_active)
            return;

        /* We insist on a fixed size packet. No overflows. */
        if (bufsize != RAW_PACKET_LENGTH)
            return;

        uart_packet_t *packet = (uart_packet_t *) (buffer + START_LENGTH);

        /* Only a certain packet types are accepted */
        if (!validate_packet(packet))
            return;

        process_packet(packet, &global_state);
    }

    /* LED reports use REPORT_ID_KEYBOARD, or 0 in boot protocol (no report ID byte). */
    if (report_id != REPORT_ID_KEYBOARD && report_id != REPORT_ID_NONE)
        return;

    /* The LED report is exactly one byte of "output" type. */
    if (bufsize != 1 || report_type != HID_REPORT_TYPE_OUTPUT)
        return;

    uint8_t leds = buffer[0];

    /* If we are using caps lock LED to indicate the chosen output, that has priority */
    if (global_state.config.kbd_led_as_indicator) {
        leds = leds & 0xFD; /* 1111 1101 (Clear Caps Lock bit) */

        if (global_state.active_output)
            leds |= KEYBOARD_LED_CAPSLOCK;
    }

    global_state.keyboard_leds_desired[BOARD_ROLE] = leds;

    /* If the board has a keyboard connected directly, restore those leds. */
    if (global_state.keyboard_connected && CURRENT_BOARD_IS_ACTIVE_OUTPUT)
        restore_leds(&global_state);

    /* Always send to the other one, so it is aware of the change */
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

#ifdef DH_DEBUG_CDC_FLASH
void tud_cdc_rx_cb(uint8_t itf) {
    char buf[64];
    uint32_t count = tud_cdc_n_available(itf);

    if (count == 0)
        return;

    if (count > sizeof(buf))
        count = sizeof(buf);

    tud_cdc_n_read(itf, buf, count);

    if (count >= 5 && memcmp(buf, "flash", 5) == 0) {
        reset_usb_boot(0, 0);
    }
}
#endif

/* ================================================== *
 * ===============  USB HOST Section  =============== *
 * ================================================== */

static void hidpp_unmount(uint8_t dev_addr, uint8_t instance);

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    hidpp_unmount(dev_addr, instance);
    uint8_t itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

    if (dev_addr > MAX_DEVICES || instance >= MAX_INTERFACES)
        return;

    hid_interface_t *iface = &global_state.iface[dev_addr-1][instance];

    switch (itf_protocol) {
        case HID_ITF_PROTOCOL_KEYBOARD:
            global_state.keyboard_connected = false;

            /* Clear local keyboard state so unplugging doesn't leave keys stuck */
            memset(&global_state.local_kbd_states, 0, sizeof(global_state.local_kbd_states));
            break;

        case HID_ITF_PROTOCOL_MOUSE:
            global_state.mouse_connected = false;
            break;
    }

    /* Also clear the interface structure, otherwise plugging something else later
       might be a fun (and confusing) experience */
    memset(iface, 0, sizeof(hid_interface_t));
}

/* ==================================================== *
 * Logitech HID++ support: thumb and top mouse buttons
 * These buttons only reach DeskHop through Logitech's
 * private protocol on the receiver's vendor interface.
 * ==================================================== */

#define HIDPP_DEVICE_INDEX 2    /* Mouse index on the receiver (found on the test Mac) */
#define HIDPP_REPROG_INDEX 9    /* REPROG_CONTROLS_V4 feature index on that mouse */
#define HIDPP_CID_THUMB    0x00C3
#define HIDPP_CID_TOP      0x00C4
#define HIDPP_RESEND_US    2000000

static uint8_t hidpp_dev_addr      = 0;     /* 0 = no HID++ interface found yet */
static uint8_t hidpp_instance      = 0;
static uint8_t hidpp_pending = 0; /* One confirmation bit per button. */
static uint16_t hidpp_waiting_cid = 0;
static bool hidpp_tx_busy = false;
static uint8_t hidpp_tx_report[20];
static uint64_t mac_clipboard_due = 0;
static uint8_t mac_clipboard_output;
static void hidpp_send_shortcut(device_t *state, uint8_t key);

/* Finish the Spotlight sequence without blocking USB polling. Never send the
 * second shortcut to a different output after a computer switch. */
static void mac_clipboard_task(device_t *state) {
    if (!mac_clipboard_due)
        return;
    if (state->active_output != mac_clipboard_output ||
        state->config.output[mac_clipboard_output].os != MACOS) {
        mac_clipboard_due = 0;
        return;
    }
    if (time_us_64() >= mac_clipboard_due) {
        mac_clipboard_due = 0;
        hidpp_send_shortcut(state, HID_KEY_4);
    }
}
static uint64_t hidpp_next_send_us = 0;
static uint8_t hidpp_pressed_prev  = 0;

static void hidpp_unmount(uint8_t dev_addr, uint8_t instance) {
    if (dev_addr != hidpp_dev_addr || instance != hidpp_instance)
        return;
    hidpp_dev_addr = 0;
    hidpp_tx_busy = false;
    hidpp_pending = 0;
    hidpp_waiting_cid = 0;
    hidpp_pressed_prev = 0;
}


/* A receiver's HID++ interface starts with usage page 0xFF00 and declares report ID 0x11 */
static bool is_hidpp_interface(uint8_t const *desc, uint16_t len) {
    if (len < 3 || desc[0] != 0x06 || desc[1] != 0x00 || desc[2] != 0xFF)
        return false;

    /* Two HID++ interfaces exist (short and long reports). We want the long one, usage 0x02. */
    bool has_report_11 = false;
    bool has_usage_2   = false;
    for (uint16_t i = 0; i + 1 < len; i++) {
        if (desc[i] == 0x85 && desc[i + 1] == 0x11)
            has_report_11 = true;
        if (desc[i] == 0x09 && desc[i + 1] == 0x02)
            has_usage_2 = true;
    }
    return has_report_11 && has_usage_2;
}

/* Ask the mouse to send a button's presses to DeskHop instead of hiding them */
/* Control SET_REPORT supports receivers without an interrupt OUT endpoint.
 * TinyUSB retains this buffer until its completion callback. */
void tuh_hid_set_report_complete_cb(uint8_t dev_addr, uint8_t instance,
                                  uint8_t report_id, uint8_t report_type, uint16_t len) {
    if (dev_addr != hidpp_dev_addr || instance != hidpp_instance ||
        report_id != 0x11 || report_type != HID_REPORT_TYPE_OUTPUT)
        return;
    hidpp_tx_busy = false;
    if (len != sizeof(hidpp_tx_report))
        hidpp_next_send_us = time_us_64() + 10000;
}

static bool hidpp_send_divert(uint16_t cid) {
    if (hidpp_tx_busy)
        return false;

    memset(hidpp_tx_report, 0, sizeof(hidpp_tx_report));
    hidpp_tx_report[0] = 0x11;
    hidpp_tx_report[1] = HIDPP_DEVICE_INDEX;
    hidpp_tx_report[2] = HIDPP_REPROG_INDEX;
    hidpp_tx_report[3] = 0x31;
    hidpp_tx_report[4] = cid >> 8;
    hidpp_tx_report[5] = cid & 0xFF;
    hidpp_tx_report[6] = 0x03; /* divert + diversion-valid bit */

    hidpp_tx_busy = true;
    if (!tuh_hid_set_report(hidpp_dev_addr, hidpp_instance, 0x11,
                            HID_REPORT_TYPE_OUTPUT, hidpp_tx_report,
                            sizeof(hidpp_tx_report))) {
        hidpp_tx_busy = false;
        return false;
    }
    return true;
}

/* Keep asking until the mouse confirms, and again after it reconnects */
void hidpp_task(void) {
    mac_clipboard_task(&global_state);
    if (!hidpp_pending || hidpp_dev_addr == 0)
        return;

    uint64_t now = time_us_64();
    if (now < hidpp_next_send_us)
        return;

    uint16_t cid = (hidpp_pending & 1) ? HIDPP_CID_THUMB : HIDPP_CID_TOP;
    if (hidpp_send_divert(cid)) {
        hidpp_waiting_cid = cid;
        hidpp_next_send_us = now + HIDPP_RESEND_US;
    } else {
        /* Endpoint busy: retry later without losing this button. */
        hidpp_next_send_us = now + 10000;
    }
}

static void hidpp_send_shortcut(device_t *state, uint8_t key) {
    hid_keyboard_report_t press   = {0};
    hid_keyboard_report_t release = {0};

    press.modifier   = KEYBOARD_MODIFIER_LEFTGUI;
    press.keycode[0] = key;

    send_key(&press, state);
    send_key(&release, state);
}

static void hidpp_handle_report(device_t *state, uint8_t const *r, uint16_t len) {
    if (len < 7 || r[1] != HIDPP_DEVICE_INDEX)
        return;

    /* Receiver says a device connected: ask again for the buttons */
    if (r[2] == 0x41) {
        hidpp_pending = 3;
        hidpp_waiting_cid = 0;
        hidpp_pressed_prev = 0;
        hidpp_next_send_us   = 0;
        return;
    }

    if (r[2] != HIDPP_REPROG_INDEX)
        return;

    /* Confirm only the requested CID; the other button stays pending. */
    if (r[3] == 0x31) {
        uint16_t cid = ((uint16_t)r[4] << 8) | r[5];
        if (cid != hidpp_waiting_cid || (r[6] & 1) == 0)
            return;
        if (cid == HIDPP_CID_THUMB) hidpp_pending &= ~1u;
        if (cid == HIDPP_CID_TOP) hidpp_pending &= ~2u;
        hidpp_waiting_cid = 0;
        hidpp_next_send_us = 0;
#ifdef HIDPP_DEBUG
        blink_led(state);           /* stage 2: mouse confirmed the request */
#endif
        return;
    }

    /* Button event: up to four pressed CIDs, zero means released */
    if (r[3] == 0x00) {
        uint8_t pressed = 0;
        for (uint16_t i = 4; i + 1 < len && i <= 10; i += 2) {
            uint16_t cid = (r[i] << 8) | r[i + 1];
            if (cid == HIDPP_CID_THUMB) pressed |= 0x01;
            if (cid == HIDPP_CID_TOP)   pressed |= 0x02;
        }

        uint8_t newly_pressed = pressed & ~hidpp_pressed_prev;
        hidpp_pressed_prev    = pressed;
#ifdef HIDPP_DEBUG
        if (newly_pressed)
            blink_led(state);       /* stage 3: a new button press arrived */
#endif

        /* Follow the active output's OS selection from the config page. */
        if (newly_pressed & 0x01)
            toggle_gaming_mode_handler(state, NULL);
        if (newly_pressed & 0x02) {
            if (state->config.output[state->active_output].os == MACOS) {
                /* Ignore repeated clicks while Spotlight is opening. */
                if (!mac_clipboard_due) {
                    hidpp_send_shortcut(state, HID_KEY_SPACE);
                    mac_clipboard_output = state->active_output;
                    mac_clipboard_due = time_us_64() + 500000;
                }
            } else {
                hidpp_send_shortcut(state, HID_KEY_V);
            }
        }
    }
}

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len) {
    uint8_t itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

    if (is_hidpp_interface(desc_report, desc_len)) {
        hidpp_dev_addr       = dev_addr;
        hidpp_instance       = instance;
        hidpp_pending = 3;
        hidpp_waiting_cid = 0;
        hidpp_pressed_prev = 0;
        hidpp_next_send_us   = 0;
#ifdef HIDPP_DEBUG
        blink_led(&global_state);   /* stage 1: found the mouse's private interface */
#endif
    }

    if (dev_addr > MAX_DEVICES || instance >= MAX_INTERFACES)
        return;

    /* Get interface information */
    hid_interface_t *iface = &global_state.iface[dev_addr-1][instance];

    iface->protocol = tuh_hid_get_protocol(dev_addr, instance);

    /* Parse the report descriptor into our internal structure. */
    parse_report_descriptor(iface, desc_report, desc_len);

    switch (itf_protocol) {
        case HID_ITF_PROTOCOL_KEYBOARD:
            if (global_state.config.enforce_ports && BOARD_ROLE == OUTPUT_B)
                return;

            if (global_state.config.force_kbd_boot_protocol)
                tuh_hid_set_protocol(dev_addr, instance, HID_PROTOCOL_BOOT);

            /* Keeping this is required for setting leds from device set_report callback */
            global_state.kbd_dev_addr       = dev_addr;
            global_state.kbd_instance       = instance;
            global_state.keyboard_connected = true;
            break;

        case HID_ITF_PROTOCOL_MOUSE:
            if (global_state.config.enforce_ports && BOARD_ROLE == OUTPUT_A)
                return;

            if (global_state.config.force_mouse_boot_mode) {
                /* User requested boot mode - simpler protocol for compatibility.
                   Note: many mice still send wheel data even in boot mode. */
                tuh_hid_set_protocol(dev_addr, instance, HID_PROTOCOL_BOOT);
            } else {
                /* Switch to using report protocol instead of boot, it's more complicated but
                   at least we get all the information we need (looking at you, mouse wheel) */
                if (tuh_hid_get_protocol(dev_addr, instance) == HID_PROTOCOL_BOOT) {
                    tuh_hid_set_protocol(dev_addr, instance, HID_PROTOCOL_REPORT);
                }
            }
            global_state.mouse_connected = true;
            break;

        case HID_ITF_PROTOCOL_NONE:
            break;
    }

    /* Also set mouse_connected if report descriptor contains mouse, even if interface
       protocol says keyboard. This handles composite devices like QMK. */
    if (iface->mouse.is_found) {
        global_state.mouse_connected = true;
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

    if (dev_addr > MAX_DEVICES || instance >= MAX_INTERFACES)
        return;

    /* Logitech HID++ interface: handle here, nothing else to do with it */
    if (hidpp_dev_addr != 0 && dev_addr == hidpp_dev_addr && instance == hidpp_instance) {
        if (len >= 1 && report[0] == 0x11)
            hidpp_handle_report(&global_state, report, len);

        tuh_hid_receive_report(dev_addr, instance);
        return;
    }

    hid_interface_t *iface = &global_state.iface[dev_addr-1][instance];

    /* Calculate a device index that distinguishes between different devices
       while staying within the bounds of MAX_DEVICES.

       Device index assignment:
       - 0: Primary keyboard (the one set in tuh_hid_mount_cb)
       - 1: Mouse devices
       - MAX_DEVICES-2: Secondary keyboards (e.g., wireless keyboard through unified dongle)
       - (dev_addr-1) % (MAX_DEVICES-1): Other devices

       Note: Slot MAX_DEVICES-1 is reserved for the remote device (used in handle_keyboard_uart_msg) */
    uint8_t device_idx;

    if (itf_protocol == HID_ITF_PROTOCOL_KEYBOARD) {
        if (dev_addr == global_state.kbd_dev_addr && instance == global_state.kbd_instance) {
            /* Primary keyboard */
            device_idx = 0;
        } else {
            /* Secondary keyboard (e.g., wireless keyboard through unified dongle) */
            device_idx = (MAX_DEVICES - 2);
        }
    } else if (itf_protocol == HID_ITF_PROTOCOL_MOUSE) {
        /* Mouse devices */
        device_idx = 1;
    } else {
        /* Other devices */
        device_idx = (dev_addr - 1) % (MAX_DEVICES - 1);
    }

    /* Boot protocol reports omit descriptor report IDs. */
    bool report_has_id = iface->uses_report_id && iface->protocol != HID_PROTOCOL_BOOT;

    /* If the report has an ID, use its report handler. If not, choose by interface protocol. */
    if (report_has_id || itf_protocol == HID_ITF_PROTOCOL_NONE) {
        uint8_t report_id = report_has_id ? report[0] : REPORT_ID_NONE;

        process_report_f receiver = report_receivers[iface->report_handler[report_id]];

        if (receiver != NULL)
            receiver((uint8_t *)report, len, device_idx, iface);

#ifdef BUTTON_PROBE
        /* Test build: blink when a report arrives that DeskHop doesn't handle (e.g. Logitech HID++) */
        else if (report_has_id || itf_protocol == HID_ITF_PROTOCOL_NONE)
            blink_led(&global_state);
#endif
    }
    else if (itf_protocol == HID_ITF_PROTOCOL_KEYBOARD) {
        process_keyboard_report((uint8_t *)report, len, device_idx, iface);
    }
    else if (itf_protocol == HID_ITF_PROTOCOL_MOUSE) {
        process_mouse_report((uint8_t *)report, len, device_idx, iface);
    }

    /* Continue requesting reports */
    tuh_hid_receive_report(dev_addr, instance);
}

/* Set protocol in a callback. This is tied to an interface, not a specific report ID */
void tuh_hid_set_protocol_complete_cb(uint8_t dev_addr, uint8_t idx, uint8_t protocol) {
    if (dev_addr > MAX_DEVICES || idx >= MAX_INTERFACES)
        return;

    hid_interface_t *iface = &global_state.iface[dev_addr-1][idx];
    iface->protocol = protocol;
}
