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

#ifndef USB_DESCRIPTORS_H_
#define USB_DESCRIPTORS_H_

extern tusb_desc_device_t const desc_device_hid;
extern tusb_desc_device_t const desc_device_dfu;

extern uint8_t const desc_configuration_hid[];
extern uint8_t const desc_configuration_dfu[];

// String Descriptor Index
enum {
    STRID_LANGID = 0,
    STRID_MANUFACTURER,
    STRID_PRODUCT,
    STRID_SERIAL,
    STRID_MOUSE,
    STRID_DFU_RUNTIME,
    STRID_DFU_BOARD_A_FW,
    STRID_DFU_BOARD_B_FW,
    STRID_DFU_CONFIG,
    STRID_DFU_DFU_BUFFER,
#ifdef DH_DEBUG
    STRID_DEBUG,
#endif
};

enum
{
  REPORT_ID_KEYBOARD = 1,
  REPORT_ID_MOUSE,
  REPORT_ID_COUNT
};

enum
{
  REPORT_ID_RELMOUSE = 1,
};

#define TUD_HID_REPORT_DESC_ABSMOUSE(...) \
HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP      )                   ,\
HID_USAGE      ( HID_USAGE_DESKTOP_MOUSE     )                   ,\
HID_COLLECTION ( HID_COLLECTION_APPLICATION  )                   ,\
  /* Report ID */\
  __VA_ARGS__ \
  HID_USAGE      ( HID_USAGE_DESKTOP_POINTER )                   ,\
  HID_COLLECTION ( HID_COLLECTION_PHYSICAL   )                   ,\
    HID_USAGE_PAGE  ( HID_USAGE_PAGE_BUTTON  )                   ,\
      HID_USAGE_MIN   ( 1                                      ) ,\
      HID_USAGE_MAX   ( 5                                      ) ,\
      HID_LOGICAL_MIN ( 0                                      ) ,\
      HID_LOGICAL_MAX ( 1                                      ) ,\
      \
      /* Left, Right, Middle, Backward, Forward buttons */ \
      HID_REPORT_COUNT( 5                                      ) ,\
      HID_REPORT_SIZE ( 1                                      ) ,\
      HID_INPUT       ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
      \
      /* 3 bit padding */ \
      HID_REPORT_COUNT( 1                                      ) ,\
      HID_REPORT_SIZE ( 3                                      ) ,\
      HID_INPUT       ( HID_CONSTANT                           ) ,\
    HID_USAGE_PAGE  ( HID_USAGE_PAGE_DESKTOP )                   ,\
      \
      /* X, Y absolute position [0, 32767] */ \
      HID_USAGE       ( HID_USAGE_DESKTOP_X                    ) ,\
      HID_USAGE       ( HID_USAGE_DESKTOP_Y                    ) ,\
      HID_LOGICAL_MIN  ( 0x00                                ) ,\
      HID_LOGICAL_MAX_N( 0x7FFF, 2                           ) ,\
      HID_REPORT_SIZE  ( 16                                  ) ,\
      HID_REPORT_COUNT ( 2                                   ) ,\
      HID_INPUT       ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
      \
      /* Vertical wheel scroll [-127, 127] */ \
      HID_USAGE       ( HID_USAGE_DESKTOP_WHEEL                )  ,\
      HID_LOGICAL_MIN ( 0x81                                   )  ,\
      HID_LOGICAL_MAX ( 0x7f                                   )  ,\
      HID_REPORT_COUNT( 1                                      )  ,\
      HID_REPORT_SIZE ( 8                                      )  ,\
      HID_INPUT       ( HID_DATA | HID_VARIABLE | HID_RELATIVE )  ,\
      \
      /* Mouse mode (0 = absolute, 1 = relative) */ \
      HID_REPORT_COUNT( 1                                      ), \
      HID_REPORT_SIZE ( 8                                      ), \
      HID_INPUT       ( HID_CONSTANT                           ), \
  HID_COLLECTION_END                                            , \
HID_COLLECTION_END \

#endif /* USB_DESCRIPTORS_H_ */
