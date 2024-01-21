/*
 * This file is part of DeskHop (https://github.com/hrvach/deskhop).
 * Copyright (c) 2024 Hrvoje Cavrak
 *
 * Based on the TinyUSB HID parser routine and the amazing USB2N64
 * adapter (https://github.com/pdaxrom/usb2n64-adapter)
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

#define IS_BLOCK_END (collection.start == collection.end)
#define MAX_BUTTONS  16

enum { SIZE_0_BIT = 0, SIZE_8_BIT = 1, SIZE_16_BIT = 2, SIZE_32_BIT = 3 };

/* Size is 0, 1, 2, or 3, describing cases of no data, 8-bit, 16-bit,
  or 32-bit data. */
uint32_t get_descriptor_value(uint8_t const *report, int size) {
    switch (size) {
        case SIZE_8_BIT:
            return report[0];
        case SIZE_16_BIT:
            return tu_u16(report[1], report[0]);
        case SIZE_32_BIT:
            return tu_u32(report[3], report[2], report[1], report[0]);
        default:
            return 0;
    }
}

/* We store all globals as unsigned to avoid countless switch/cases.
In case of e.g. min/max, we need to treat some data as signed retroactively. */
int32_t to_signed(globals_t *data) {
    switch (data->hdr.size) {
        case SIZE_8_BIT:
            return (int8_t)data->val;
        case SIZE_16_BIT:
            return (int16_t)data->val;
        default:
            return data->val;
    }
}

/* Given a value struct with size and offset in bits,
   find and return a value from the HID report */

int32_t get_report_value(uint8_t *report, report_val_t *val) {
    /* Calculate the bit offset within the byte */
    uint8_t offset_in_bits = val->offset % 8;

    /* Calculate the remaining bits in the first byte */
    uint8_t remaining_bits = 8 - offset_in_bits;

    /* Calculate the byte offset in the array */
    uint8_t byte_offset = val->offset >> 3;

    /* Create a mask for the specified number of bits */
    uint32_t mask = (1u << val->size) - 1;

    /* Initialize the result value with the bits from the first byte */
    int32_t result = report[byte_offset] >> offset_in_bits;

    /* Move to the next byte and continue fetching bits until the desired length is reached */
    while (val->size > remaining_bits) {
        result |= report[++byte_offset] << remaining_bits;
        remaining_bits += 8;
    }

    /* Apply the mask to retain only the desired number of bits */
    result = result & mask;

    /* Special case if result is negative.
       Check if the most significant bit of 'val' is set */
    if (result & ((mask >> 1) + 1)) {
        /* If it is set, sign-extend 'val' by filling the higher bits with 1s */
        result |= (0xFFFFFFFFU << val->size);
    }

    return result;
}

void update_usage(parser_state_t *parser, int i) {
    /* If we don't have as many usages as elements, the usage for the previous element applies */
    if (i && i >= parser->usage_count) {
        *(parser->p_usage + i) = *(parser->p_usage + parser->usage_count - 1);
    }
}

void find_and_store_element(parser_state_t *parser, int map_len, int i) {
    usage_map_t *map = &parser->map[0];

    for (int j = 0; j < map_len; j++, map++) {
        /* Filter based on usage criteria */
        if (map->report_usage == parser->global_usage
            && map->usage_page == parser->globals[RI_GLOBAL_USAGE_PAGE].val
            && map->usage == *(parser->p_usage + i)) {

            /* Buttons are the ones that appear multiple times, aggregate for now */
            if (map->element->size) {
                map->element->size++;
                continue;
            }

            /* Store the found element's attributes */
            map->element->offset = parser->offset_in_bits;
            map->element->size   = parser->globals[RI_GLOBAL_REPORT_SIZE].val;
            map->element->min    = to_signed(&parser->globals[RI_GLOBAL_LOGICAL_MIN]);
            map->element->max    = to_signed(&parser->globals[RI_GLOBAL_LOGICAL_MAX]);
        }
    }
}

void handle_global_item(parser_state_t *parser, header_t *header, uint32_t data, mouse_t *mouse) {
    /* There are just 16 possible tags, store any one that comes along to an array
        instead of doing switch and 16 cases */
    parser->globals[header->tag].val = data;
    parser->globals[header->tag].hdr = *header;

    if (header->tag == RI_GLOBAL_REPORT_ID) {
        /* Important to track, if report IDs are used reports are preceded/offset by a 1-byte ID value */
        if (parser->global_usage == HID_USAGE_DESKTOP_MOUSE)
            mouse->report_id = data;

        mouse->uses_report_id = true;
    }
}

void handle_local_item(parser_state_t *parser, header_t *header, uint32_t data) {
    if (header->tag == RI_LOCAL_USAGE) {
        /* If we are not within a collection, the usage tag applies to the entire section */
        if (parser->collection.start == parser->collection.end) {
            parser->global_usage = data;
        } else {
            *(parser->p_usage + parser->usage_count++) = data;
        }
    }
}

void handle_main_item(parser_state_t *parser, header_t *header, int map_len) {
    /* Update Collection */
    parser->collection.start += (header->tag == RI_MAIN_COLLECTION);
    parser->collection.end += (header->tag == RI_MAIN_COLLECTION_END);

    if (header->tag == RI_MAIN_INPUT) {
        for (int i = 0; i < parser->globals[RI_GLOBAL_REPORT_COUNT].val; i++) {
            update_usage(parser, i);
            find_and_store_element(parser, map_len, i);

            /* Iterate <count> times and increase offset by <size> amount, moving by <count> x <size> bits */
            parser->offset_in_bits += parser->globals[RI_GLOBAL_REPORT_SIZE].val;
        }

        /* Advance the usage array pointer by global report count and reset the count variable */
        parser->p_usage += parser->globals[RI_GLOBAL_REPORT_COUNT].val;
        parser->usage_count = 0;
    }
}


/* This method is sub-optimal and far from a generalized HID descriptor parsing, but should
 * hopefully work well enough to find the basic values we care about to move the mouse around.
 * Your descriptor for a mouse with 2 wheels and 264 buttons might not parse correctly.
 **/
uint8_t parse_report_descriptor(mouse_t *mouse, uint8_t arr_count, uint8_t const *report, uint16_t desc_len) {
    usage_map_t usage_map[] = {
        {.report_usage = HID_USAGE_DESKTOP_MOUSE,
         .usage_page   = HID_USAGE_PAGE_BUTTON,
         .usage        = HID_USAGE_DESKTOP_POINTER,
         .element      = &mouse->buttons},

        {.report_usage = HID_USAGE_DESKTOP_MOUSE,
         .usage_page   = HID_USAGE_PAGE_DESKTOP,
         .usage        = HID_USAGE_DESKTOP_X,
         .element      = &mouse->move_x},

        {.report_usage = HID_USAGE_DESKTOP_MOUSE,
         .usage_page   = HID_USAGE_PAGE_DESKTOP,
         .usage        = HID_USAGE_DESKTOP_Y,
         .element      = &mouse->move_y},

        {.report_usage = HID_USAGE_DESKTOP_MOUSE,
         .usage_page   = HID_USAGE_PAGE_DESKTOP,
         .usage        = HID_USAGE_DESKTOP_WHEEL,
         .element      = &mouse->wheel},
    };

    parser_state_t parser = {0};
    parser.p_usage        = parser.usages;
    parser.map            = usage_map;

    while (desc_len > 0) {
        header_t header = *(header_t *)report++;
        uint32_t data   = get_descriptor_value(report, header.size);

        switch (header.type) {
            case RI_TYPE_MAIN:
                handle_main_item(&parser, &header, ARRAY_SIZE(usage_map));
                break;

            case RI_TYPE_GLOBAL:
                handle_global_item(&parser, &header, data, mouse);
                break;

            case RI_TYPE_LOCAL:
                handle_local_item(&parser, &header, data);
                break;
        }
        /* Move to the next position and decrement size by header length + data length */
        report += header.size;
        desc_len -= header.size + 1;
    }
    return 0;
}
