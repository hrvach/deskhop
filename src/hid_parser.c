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

#define IS_BLOCK_END (parser->collection.start == parser->collection.end)

enum { SIZE_0_BIT = 0, SIZE_8_BIT = 1, SIZE_16_BIT = 2, SIZE_32_BIT = 3 };
const uint8_t SIZE_LOOKUP[4] = {0, 1, 2, 4};

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

void update_usage(parser_state_t *parser, int i) {
    /* If we don't have as many usages as elements, the usage for the previous element applies */
    if (i > 0 && i >= parser->usage_count && i < HID_MAX_USAGES)
        *(parser->p_usage + i) = *(parser->p_usage + i - 1);
}

void store_element(parser_state_t *parser, report_val_t *val, int i, uint32_t data, uint16_t size, hid_interface_t *iface) {   
    *val = (report_val_t){
        .offset     = parser->offset_in_bits,
        .offset_idx = parser->offset_in_bits >> 3,
        .size       = size,

        .usage_max = parser->locals[RI_LOCAL_USAGE_MAX].val,
        .usage_min = parser->locals[RI_LOCAL_USAGE_MIN].val,

        .item_type   = (data & 0x01) ? CONSTANT : DATA,
        .data_type   = (data & 0x02) ? VARIABLE : ARRAY,

        .usage        = *(parser->p_usage + i),
        .usage_page   = parser->globals[RI_GLOBAL_USAGE_PAGE].val,
        .global_usage = parser->global_usage,
        .report_id    = parser->report_id
    };

    iface->uses_report_id |= (parser->report_id != 0);
}

void handle_global_item(parser_state_t *parser, item_t *item) {
    if (item->hdr.tag == RI_GLOBAL_REPORT_ID) 
        parser->report_id = item->val;        
    
    parser->globals[item->hdr.tag] = *item;
}

void handle_local_item(parser_state_t *parser, item_t *item) {
    /* There are just 16 possible tags, store any one that comes along to an array
        instead of doing switch and 16 cases */
    parser->locals[item->hdr.tag] = *item;

    if (item->hdr.tag == RI_LOCAL_USAGE) {
        if(IS_BLOCK_END) 
            parser->global_usage = item->val;

        else if (parser->usage_count < HID_MAX_USAGES - 1)
            *(parser->p_usage + parser->usage_count++) = item->val;
    }
}

void handle_main_input(parser_state_t *parser, item_t *item, hid_interface_t *iface) {
    uint32_t size  = parser->globals[RI_GLOBAL_REPORT_SIZE].val;
    uint32_t count = parser->globals[RI_GLOBAL_REPORT_COUNT].val;
    report_val_t val = {0};

    /* Swap count and size for 1-bit variables, it makes sense to process e.g. NKRO with
       size = 1 and count = 240 in one go instead of doing 240 iterations 
       Don't do this if there are usages in the queue, though.
       */           
    if (size == 1 && parser->usage_count <= 1) {
        size  = count;
        count = 1;        
    }

    for (int i = 0; i < count; i++) {
        update_usage(parser, i);
        store_element(parser, &val, i, item->val, size, iface);
        
        /* Use the parsed data to populate internal device structures */
        extract_data(iface, &val);    

        /* Iterate <count> times and increase offset by <size> amount, moving by <count> x <size> bits */
        parser->offset_in_bits += size;
    }

    /* Advance the usage array pointer by global report count and reset the count variable */
    parser->p_usage += parser->usage_count;

    /* Carry the last usage to the new location */
    *parser->p_usage = *(parser->p_usage - parser->usage_count);
}

void handle_main_item(parser_state_t *parser, item_t *item, hid_interface_t *iface) {
    if (IS_BLOCK_END)
        parser->offset_in_bits = 0;

    switch (item->hdr.tag) {
        case RI_MAIN_COLLECTION:
            parser->collection.start++;
            break;

        case RI_MAIN_COLLECTION_END:
            parser->collection.end++;
            break;

        case RI_MAIN_INPUT:
            handle_main_input(parser, item, iface);
            break;
    }

    parser->usage_count = 0;

    /* Local items do not carry over to the next Main item (HID spec v1.11, section 6.2.2.8) */
    memset(parser->locals, 0, sizeof(parser->locals));
}


/* This method is sub-optimal and far from a generalized HID descriptor parsing, but should
 * hopefully work well enough to find the basic values we care about to move the mouse around.
 * Your descriptor for a mouse with 2 wheels and 264 buttons might not parse correctly.
 **/
parser_state_t parser_state = {0};  // Avoid placing it on the stack, it's large

void parse_report_descriptor(hid_interface_t *iface,
                            uint8_t const *report,                            
                            int desc_len
                            ) {
    item_t item = {0};

    /* Wipe parser_state clean */
    memset(&parser_state, 0, sizeof(parser_state_t));
    parser_state.p_usage = parser_state.usages;

    while (desc_len > 0) {
        item.hdr = *(header_t *)report++;
        item.val = get_descriptor_value(report, item.hdr.size);

        switch (item.hdr.type) {
            case RI_TYPE_MAIN:
                handle_main_item(&parser_state, &item, iface);
                break;

            case RI_TYPE_GLOBAL:
                handle_global_item(&parser_state, &item);
                break;

            case RI_TYPE_LOCAL:
                handle_local_item(&parser_state, &item);
                break;
        }
        /* Move to the next position and decrement size by header length + data length */
        report += SIZE_LOOKUP[item.hdr.size];
        desc_len -= (SIZE_LOOKUP[item.hdr.size] + 1);
    }    
}
