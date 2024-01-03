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
#include "hid_parser.h"

#define IS_BLOCK_END (collection.start == collection.end)
#define MAX_BUTTONS 16

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

int32_t get_report_value(uint8_t* report, report_val_t *val) {    
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

/* This method is far from a generalized HID descriptor parsing, but should work 
 * well enough to find the basic values we care about to move the mouse around.
 * Your descriptor for a mouse with 2 wheels and 264 buttons might not parse correctly. 
 **/
uint8_t parse_report_descriptor(mouse_t *mouse, uint8_t arr_count,
                                uint8_t const *report, uint16_t desc_len) {

    /* Get these elements and store them in the proper place in the mouse struct
     * For example, to match wheel, we want collection usage to be HID_USAGE_DESKTOP_MOUSE, page to be HID_USAGE_PAGE_DESKTOP,
     * usage to be HID_USAGE_DESKTOP_WHEEL, then if all of that is matched we store the value to mouse->wheel */
    const usage_map_t usage_map[] = {
        {HID_USAGE_DESKTOP_MOUSE, HID_USAGE_PAGE_BUTTON, HID_USAGE_DESKTOP_POINTER, &mouse->buttons},
        {HID_USAGE_DESKTOP_MOUSE, HID_USAGE_PAGE_DESKTOP, HID_USAGE_DESKTOP_X, &mouse->move_x},
        {HID_USAGE_DESKTOP_MOUSE, HID_USAGE_PAGE_DESKTOP, HID_USAGE_DESKTOP_Y, &mouse->move_y},
        {HID_USAGE_DESKTOP_MOUSE, HID_USAGE_PAGE_DESKTOP, HID_USAGE_DESKTOP_WHEEL, &mouse->wheel},
    };

  /* Some variables used for keeping tabs on parsing */
  uint8_t usage_count = 0;
  uint8_t g_usage = 0;
  
  uint32_t offset_in_bits = 0;

  uint8_t usages[64] = {0};
  uint8_t* p_usage = usages;

  collection_t collection = {0};

  /* as tag is 4 bits, there can be 16 different tags in global header type */
  globals_t globals[16] = {0}; 

  for (int len = desc_len; len > 0; len--) {
    header_t header = *(header_t *)report++;
    uint32_t data = get_descriptor_value(report, header.size);

    switch (header.type) {
    case RI_TYPE_MAIN:
      // Keep count of collections, starts and ends
      collection.start += (header.tag == RI_MAIN_COLLECTION);
      collection.end += (header.tag == RI_MAIN_COLLECTION_END);
      
      if (header.tag == RI_MAIN_INPUT) {            
        for (int i = 0; i < globals[RI_GLOBAL_REPORT_COUNT].val; i++) {
          
          /* If we don't have as many usages as elements, the usage for the previous
             element applies */
          if (i && i >= usage_count ) {
              *(p_usage + i) = *(p_usage + usage_count - 1);
          }

          const usage_map_t *map = usage_map;

          /* Only focus on the items we care about (buttons, x and y, wheels, etc) */
          for (int j=0; j<sizeof(usage_map)/sizeof(usage_map[0]); j++, map++) {   
            /* Filter based on usage criteria */
            if (map->report_usage == g_usage &&
                map->usage_page == globals[RI_GLOBAL_USAGE_PAGE].val && 
                map->usage == *(p_usage + i)) {
                    
                    /* Buttons are the ones that appear multiple times, will handle them properly
                       For now, let's just aggregate the length and combine them into one :) */
                    if (map->element->size) {
                        map->element->size++;
                        continue;
                    }

                    /* Store the found element's attributes */
                    map->element->offset = offset_in_bits;
                    map->element->size = globals[RI_GLOBAL_REPORT_SIZE].val;
                    map->element->min = to_signed(&globals[RI_GLOBAL_LOGICAL_MIN]);
                    map->element->max = to_signed(&globals[RI_GLOBAL_LOGICAL_MAX]);
                }
          };
          
          /* Iterate <count> times and increase offset by <size> amount, moving by <count> x <size> bits */
          offset_in_bits += globals[RI_GLOBAL_REPORT_SIZE].val;
        }
        /* Advance the usage array pointer by global report count and reset the count variable */
        p_usage += globals[RI_GLOBAL_REPORT_COUNT].val;
        usage_count = 0;        
      }
      break;

    case RI_TYPE_GLOBAL:
      /* There are just 16 possible tags, store any one that comes along to an array instead of doing 
         switch and 16 cases */
      globals[header.tag].val = data;
      globals[header.tag].hdr = header;

      if (header.tag == RI_GLOBAL_REPORT_ID) {
        /* Important to track, if report IDs are used reports are preceded/offset by a 1-byte ID value */
        if(g_usage == HID_USAGE_DESKTOP_MOUSE)
          mouse->report_id = data;
        
        mouse->uses_report_id = true;        
      }
      break;

    case RI_TYPE_LOCAL:
      if (header.tag == RI_LOCAL_USAGE) {
        /* If we are not within a collection, the usage tag applies to the entire section */
        if (IS_BLOCK_END)
          g_usage = data;
        else
          *(p_usage + usage_count++) = data;        
      }
      break;
    }

    /* If header specified some non-zero length data, move by that much to get to the new byte
       we should interpret as a header element */
    report += header.size;
    len -= header.size;
  }
  return 0;
}
