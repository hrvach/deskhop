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
#include "hid_report.h"
#include "main.h"

/* Given a value struct with size and offset in bits, find and return a value from the HID report */
int32_t get_report_value(uint8_t *report, int len, report_val_t *val) {
    /* Calculate the bit offset within the byte */
    uint16_t offset_in_bits = val->offset % 8;

    /* Calculate the remaining bits in the first byte */
    uint16_t remaining_bits = 8 - offset_in_bits;

    /* Calculate the byte offset in the array */
    uint16_t byte_offset = val->offset >> 3;

    if (byte_offset >= len)
        return 0;

    /* Create a mask for the specified number of bits */
    uint32_t mask = (1u << val->size) - 1;

    /* Initialize the result value with the bits from the first byte */
    int32_t result = report[byte_offset] >> offset_in_bits;

    /* Move to the next byte and continue fetching bits until the desired length is reached */
    while (val->size > remaining_bits && byte_offset < len) {
        result |= report[++byte_offset] << remaining_bits;
        remaining_bits += 8;
    }

    /* Apply the mask to retain only the desired number of bits */
    result = result & mask;

    /* Special case if our result is negative.
       Check if the most significant bit of 'val' is set */
    if (result & ((mask >> 1) + 1)) {
        /* If it is set, sign-extend 'val' by filling the higher bits with 1s */
        result |= (0xFFFFFFFFU << val->size);
    }

    return result;
}

/* After processing the descriptor, assign the values so we can later use them to interpret reports */
void handle_consumer_control_values(report_val_t *src, report_val_t *dst, hid_interface_t *iface) {
    keyboard_t *keyboard = get_keyboard(iface, src->report_id);

    if (src->offset >= MAX_CC_BUTTONS) {
        return;
    }

    if (src->data_type == VARIABLE) {
        keyboard->cc_array[src->offset] = src->usage;
        iface->consumer.is_variable = true;
    }

    iface->consumer.is_array |= (src->data_type == ARRAY);
}

/* After processing the descriptor, assign the values so we can later use them to interpret reports */
void handle_system_control_values(report_val_t *src, report_val_t *dst, hid_interface_t *iface) {
    keyboard_t *keyboard = get_keyboard(iface, src->report_id);

    if (src->offset >= MAX_SYS_BUTTONS) {
        return;
    }

    if (src->data_type == VARIABLE) {
        keyboard->sys_array[src->offset] = src->usage;
        iface->system.is_variable = true;
    }

    iface->system.is_array |= (src->data_type == ARRAY);
}

static bool is_modifier_descriptor(const report_val_t *value) {
    const int left_ctrl_usage = 0xE0;

    return value->size <= MODIFIER_BIT_LENGTH
           && left_ctrl_usage >= value->usage_min
           && left_ctrl_usage <= value->usage_max;
}

static bool maps_usage_to_bitmap_bits(const report_val_t *value) {
    return value->usage_max > value->usage_min
           && (value->usage_max - value->usage_min + 1) == (int32_t)value->size;
}

static void store_modifier(keyboard_t *keyboard, const report_val_t *value) {
    if (is_modifier_descriptor(value) && value->data_type == VARIABLE)
        keyboard->modifier = *value;
}

/* An array field contains one of several possible usages, so its report position
   contains a keycode that can be copied into the 6KRO keyboard report. */
static void store_key_field(keyboard_t *keyboard, const report_val_t *value) {
    if (value->offset_idx < MAX_KEYS)
        keyboard->key_array[value->offset_idx] = value->data_type == ARRAY;
}

static void store_nkro_block(
    keyboard_t *keyboard, const report_val_t *value, bool is_modifier) {
    /* Modifier bits are stored separately, not as ordinary NKRO keys. */
    if (is_modifier)
        return;

    /* NKRO uses variable fields: one bit represents one key. */
    if (value->data_type != VARIABLE)
        return;

    /* An NKRO bitmap must contain one consecutive usage for every bit. */
    if (!maps_usage_to_bitmap_bits(value))
        return;

    /* Prevent overflowing available storage for NKRO blocks. */
    if (keyboard->nkro_count >= MAX_NKRO_BLOCKS)
        return;

    keyboard->nkro[keyboard->nkro_count++] = (nkro_block_t){
        .offset_bits = value->offset,
        .size_bits   = value->size,
        .usage_min = value->usage_min,
        .usage_max = value->usage_max,
    };
    keyboard->nkro_bit_count += value->size;
    keyboard->is_nkro = keyboard->nkro_bit_count > NKRO_MIN_BITS;
}

/* Store descriptor values so they can later be used to interpret reports. */
void handle_keyboard_descriptor_values(report_val_t *src, report_val_t *dst, hid_interface_t *iface) {
    /* Parse time: an unseen report ID claims its own keyboard_t. */
    keyboard_t *keyboard = get_or_add_keyboard(iface, src->report_id);

    /* Constants are normally used for padding, so skip'em */
    if (src->item_type == CONSTANT)
        return;

    /* Prevent overwriting more memory than we have */
    if (iface->num_keyboards >= MAX_KEYBOARDS)
        return;

    bool is_modifier = is_modifier_descriptor(src);

    store_modifier(keyboard, src);

    store_key_field(keyboard, src);
    store_nkro_block(keyboard, src, is_modifier);

    /* We found a keyboard on this interface for a specific report id. */
    if (!keyboard->is_found) {
        keyboard->is_found = true;
        iface->num_keyboards++;
    }
}

void handle_buttons(report_val_t *src, report_val_t *dst, hid_interface_t *iface) {
    /* Constant is normally used for padding with mouse buttons, aggregate to simplify things */
    if (src->item_type == CONSTANT) {
        iface->mouse.buttons.size += src->size;
        return;
    }

    iface->mouse.buttons = *src;

    /* We found a mouse on this interface. */
    iface->mouse.is_found = true;
}

void _store(report_val_t *src, report_val_t *dst, hid_interface_t *iface) {
    if (src->item_type != CONSTANT)
        *dst = *src;
}

static uint8_t *get_mouse_id(hid_interface_t *iface) {
    return &iface->mouse.report_id;
}

static uint8_t *get_consumer_id(hid_interface_t *iface) {
    return &iface->consumer.report_id;
}

static uint8_t *get_system_id(hid_interface_t *iface) {
    return &iface->system.report_id;
}

const process_report_f report_receivers[] = {
    [REPORT_RECEIVER_NONE]     = NULL,
    [REPORT_RECEIVER_MOUSE]    = process_mouse_report,
    [REPORT_RECEIVER_KEYBOARD] = process_keyboard_report,
    [REPORT_RECEIVER_CONSUMER] = process_consumer_report,
    [REPORT_RECEIVER_SYSTEM]   = process_system_report,
};

void extract_data(hid_interface_t *iface, report_val_t *val) {
    const usage_map_t map[] = {
        {.usage_page   = HID_USAGE_PAGE_BUTTON,
         .global_usage = HID_USAGE_DESKTOP_MOUSE,
         .handler      = handle_buttons,
         .receiver_id  = REPORT_RECEIVER_MOUSE,
         .dst          = &iface->mouse.buttons,
         .get_id       = get_mouse_id},

        {.usage_page   = HID_USAGE_PAGE_DESKTOP,
         .global_usage = HID_USAGE_DESKTOP_MOUSE,
         .usage        = HID_USAGE_DESKTOP_X,
         .handler      = _store,
         .receiver_id  = REPORT_RECEIVER_MOUSE,
         .dst          = &iface->mouse.move_x,
         .get_id       = get_mouse_id},

        {.usage_page   = HID_USAGE_PAGE_DESKTOP,
         .global_usage = HID_USAGE_DESKTOP_MOUSE,
         .usage        = HID_USAGE_DESKTOP_Y,
         .handler      = _store,
         .receiver_id  = REPORT_RECEIVER_MOUSE,
         .dst          = &iface->mouse.move_y,
         .get_id       = get_mouse_id},

        {.usage_page   = HID_USAGE_PAGE_DESKTOP,
         .global_usage = HID_USAGE_DESKTOP_MOUSE,
         .usage        = HID_USAGE_DESKTOP_WHEEL,
         .handler      = _store,
         .receiver_id  = REPORT_RECEIVER_MOUSE,
         .dst          = &iface->mouse.wheel,
         .get_id       = get_mouse_id},

        {.usage_page   = HID_USAGE_PAGE_CONSUMER,
         .global_usage = HID_USAGE_DESKTOP_MOUSE,
         .usage        = HID_USAGE_CONSUMER_AC_PAN,
         .handler      = _store,
         .receiver_id  = REPORT_RECEIVER_MOUSE,
         .dst          = &iface->mouse.pan,
         .get_id       = get_mouse_id},

        {.usage_page   = HID_USAGE_PAGE_KEYBOARD,
         .global_usage = HID_USAGE_DESKTOP_KEYBOARD,
         .handler      = handle_keyboard_descriptor_values,
         .receiver_id  = REPORT_RECEIVER_KEYBOARD},

        {.usage_page   = HID_USAGE_PAGE_CONSUMER,
         .global_usage = HID_USAGE_CONSUMER_CONTROL,
         .handler      = handle_consumer_control_values,
         .receiver_id  = REPORT_RECEIVER_CONSUMER,
         .dst          = &iface->consumer.val,
         .get_id       = get_consumer_id},

        {.usage_page   = HID_USAGE_PAGE_DESKTOP,
         .global_usage = HID_USAGE_DESKTOP_SYSTEM_CONTROL,
         .handler      = _store,
         .receiver_id  = REPORT_RECEIVER_SYSTEM,
         .dst          = &iface->system.val,
         .get_id       = get_system_id},
    };

    /* We extracted all we could find in the descriptor to report_values, now go through them and
       match them up with the values in the table above, then store those values for later reference */

    for (const usage_map_t *hay = map; hay != &map[ARRAY_SIZE(map)]; hay++) {
        /* ---> If any condition is not defined, we consider it as matched <--- */
        bool global_usages_match = (val->global_usage == hay->global_usage) || (hay->global_usage == 0);
        bool usages_match        = (val->usage == hay->usage) || (hay->usage == 0);
        bool usage_pages_match   = (val->usage_page == hay->usage_page) || (hay->usage_page == 0);

        if (global_usages_match && usages_match && usage_pages_match) {
            /* Keyboards claim their slot in the handler, where the report ID is known. */
            if (hay->get_id != NULL)
                *(hay->get_id(iface)) = val->report_id;

            hay->handler(val, hay->dst, iface);

            iface->report_handler[val->report_id] = hay->receiver_id;
        }
    }
}

/* Append pressed usages from one bitmap block. The report pointer excludes any report ID. */
int32_t extract_bit_variable(
    const nkro_block_t *block, const uint8_t *raw_report, int report_length, uint8_t *dst, int max_keys) {
    int key_count = 0;

    for (int block_bit = 0; block_bit < block->size_bits && key_count < max_keys; block_bit++) {
        int report_bit  = block->offset_bits + block_bit;
        int byte_index  = report_bit >> 3;
        int bit_index   = report_bit & 0b111;

        /* Report is shorter than the descriptor claims, don't read past the end of it */
        if (byte_index >= report_length)
            break;

        if (raw_report[byte_index] & (1 << bit_index)) {
            dst[key_count++] = (uint8_t)(block->usage_min + block_bit);
        }
    }

    return key_count;
}

int32_t _extract_kbd_boot(uint8_t *raw_report, int len, hid_keyboard_report_t *report) {
    uint8_t *src = raw_report;

    /* In case keyboard still uses report ID in this, just pick the last 8 bytes */
    if (len == KBD_REPORT_LENGTH + 1)
        src++;

    memcpy(report, src, KBD_REPORT_LENGTH);
    return KBD_REPORT_LENGTH;
}

int32_t _extract_kbd_other(uint8_t *raw_report, int len, hid_interface_t *iface, hid_keyboard_report_t *report) {
    keyboard_t *kb = get_keyboard(iface, raw_report[0]);
    uint8_t *src = raw_report;

    if (iface->uses_report_id)
        src++;

    if (kb->modifier.offset_idx >= len)
        return -1;

    report->modifier = src[kb->modifier.offset_idx];
    for (int i=0, j=0; i < MAX_KEYS && j < KEYS_IN_USB_REPORT; i++) {
        if(kb->key_array[i])
            report->keycode[j++] = src[i];
    }

    return KBD_REPORT_LENGTH;
}

int32_t _extract_kbd_nkro(uint8_t *raw_report, int len, hid_interface_t *iface, hid_keyboard_report_t *report) {
    keyboard_t *kb = get_keyboard(iface, raw_report[0]);
    uint8_t *ptr = raw_report;
    int key_count = 0;

    /* Skip report ID */
    if (iface->uses_report_id) {
        ptr++;
        len--;
    }

    if (kb->nkro_count == 0)
        return -1;

    /* We expect modifier to be 8 bits long, otherwise we'll fallback to boot mode */
    if (kb->modifier.size != MODIFIER_BIT_LENGTH || kb->modifier.offset_idx >= len)
        return -1;

    report->modifier = ptr[kb->modifier.offset_idx];

    /* Collect keys from every bitmap block until the outgoing 6KRO report is full */
    for (int i = 0; i < kb->nkro_count && key_count < KEYS_IN_USB_REPORT; i++) {
        key_count += extract_bit_variable(
            &kb->nkro[i], ptr, len, &report->keycode[key_count], KEYS_IN_USB_REPORT - key_count);
    }

    return key_count;
}

int32_t extract_kbd_data(
    uint8_t *raw_report, int len, uint8_t itf, hid_interface_t *iface, hid_keyboard_report_t *report) {
    keyboard_t *keyboard = get_keyboard(iface, raw_report[0]);

    /* Clear the report to start fresh */
    memset(report, 0, KBD_REPORT_LENGTH);

    /* If we're in boot protocol mode, then it's easy to decide. */
    if (iface->protocol == HID_PROTOCOL_BOOT)
        return _extract_kbd_boot(raw_report, len, report);

    /* NKRO is a special case. If extraction fails, fall through to other extractors. */
    if (keyboard->is_nkro) {
        int32_t ret = _extract_kbd_nkro(raw_report, len, iface, report);
        if (ret >= 0)
            return ret;
        memset(report, 0, KBD_REPORT_LENGTH);
    }

    /* If we're getting 8 bytes of report, it's safe to assume standard modifier + reserved + keys */
    if (!iface->uses_report_id && (len == KBD_REPORT_LENGTH || len == KBD_REPORT_LENGTH + 1))
        return _extract_kbd_boot(raw_report, len, report);

    /* This is something completely different, look at the report  */
    return _extract_kbd_other(raw_report, len, iface, report);
}
