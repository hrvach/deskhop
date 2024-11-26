/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef USB_DESCRIPTORS_H_
#define USB_DESCRIPTORS_H_

// Interface 0
#define REPORT_ID_KEYBOARD 1
#define REPORT_ID_MOUSE    2
#define REPORT_ID_CONSUMER 3
#define REPORT_ID_SYSTEM   4

// Interface 1
#define REPORT_ID_RELMOUSE  5
#define REPORT_ID_DIGITIZER 7

// Interface 2
#define REPORT_ID_VENDOR 6


#define DEVICE_DESCRIPTOR(vid, pid) \
{.bLength         = sizeof(tusb_desc_device_t),\
  .bDescriptorType = TUSB_DESC_DEVICE,\
  .bcdUSB          = 0x0200,\
  .bDeviceClass    = 0x00,\
  .bDeviceSubClass = 0x00,\
  .bDeviceProtocol = 0x00,\
  .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,\
  .idVendor  = vid,\
  .idProduct = pid,\
  .bcdDevice = 0x0100,\
  .iManufacturer = 0x01,\
  .iProduct      = 0x02,\
  .iSerialNumber = 0x03,\
  .bNumConfigurations = 0x01}\

/* Common mouse descriptor. Use HID_RELATIVE or HID_ABSOLUTE for ABS_OR_REL. */
#define TUD_HID_REPORT_DESC_MOUSE_COMMON(ABS_OR_REL, MOUSE_MIN, ...)\
  HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP      )                   ,\
  HID_USAGE      ( HID_USAGE_DESKTOP_MOUSE     )                   ,\
  HID_COLLECTION ( HID_COLLECTION_APPLICATION  )                   ,\
    /* Report ID if any */\
    __VA_ARGS__ \
    HID_USAGE      ( HID_USAGE_DESKTOP_POINTER )                   ,\
    HID_COLLECTION ( HID_COLLECTION_PHYSICAL   )                   ,\
      HID_USAGE_PAGE  ( HID_USAGE_PAGE_BUTTON  )                   ,\
        HID_USAGE_MIN   ( 1                                      ) ,\
        HID_USAGE_MAX   ( 8                                      ) ,\
        HID_LOGICAL_MIN ( 0                                      ) ,\
        HID_LOGICAL_MAX ( 1                                      ) ,\
        \
        /* Left, Right, Mid, Back, Forward buttons + 3 extra */     \
        HID_REPORT_COUNT( 8                                      ) ,\
        HID_REPORT_SIZE ( 1                                      ) ,\
        HID_INPUT       ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
        \
      HID_USAGE_PAGE  ( HID_USAGE_PAGE_DESKTOP )                   ,\
        \
        /* X, Y position [MOUSE_MIN, 32767] */ \
        HID_USAGE       ( HID_USAGE_DESKTOP_X                    ) ,\
        HID_USAGE       ( HID_USAGE_DESKTOP_Y                    ) ,\
        MOUSE_MIN                                                  ,\
        HID_LOGICAL_MAX_N( 0x7FFF, 2                             ) ,\
        HID_REPORT_SIZE  ( 16                                    ) ,\
        HID_REPORT_COUNT ( 2                                     ) ,\
        HID_INPUT       ( HID_DATA | HID_VARIABLE | ABS_OR_REL   ) ,\
        \
        /* Vertical wheel scroll [-127, 127] */ \
        HID_USAGE       ( HID_USAGE_DESKTOP_WHEEL                ) ,\
        HID_LOGICAL_MIN ( 0x81                                   ) ,\
        HID_LOGICAL_MAX ( 0x7f                                   ) ,\
        HID_REPORT_COUNT( 1                                      ) ,\
        HID_REPORT_SIZE ( 8                                      ) ,\
        HID_INPUT       ( HID_DATA | HID_VARIABLE | HID_RELATIVE ) ,\
        \
        /* Horizontal wheel (AC Pan) */ \
        HID_USAGE_PAGE  ( HID_USAGE_PAGE_CONSUMER                ) ,\
        HID_LOGICAL_MIN ( 0x81                                   ) ,\
        HID_LOGICAL_MAX ( 0x7f                                   ) ,\
        HID_REPORT_COUNT( 1                                      ) ,\
        HID_REPORT_SIZE ( 8                                      ) ,\
        HID_USAGE_N     ( HID_USAGE_CONSUMER_AC_PAN, 2           ) ,\
        HID_INPUT       ( HID_DATA | HID_VARIABLE | HID_RELATIVE ) ,\
        \
        /* Mouse mode (0 = absolute, 1 = relative) */ \
        HID_REPORT_COUNT( 1                                      ), \
        HID_REPORT_SIZE ( 8                                      ), \
        HID_INPUT       ( HID_CONSTANT                           ), \
    HID_COLLECTION_END                                            , \
  HID_COLLECTION_END \

/* Absolute mouse, range=[0..32767] */
#define TUD_HID_REPORT_DESC_ABS_MOUSE(...) TUD_HID_REPORT_DESC_MOUSE_COMMON(HID_ABSOLUTE, HID_LOGICAL_MIN(0), __VA_ARGS__)

/* Relative mouse, range=[-32767..32767] */
#define TUD_HID_REPORT_DESC_MOUSEHELP(...) TUD_HID_REPORT_DESC_MOUSE_COMMON(HID_RELATIVE, HID_LOGICAL_MIN_N(-32767, 2), __VA_ARGS__)

// Consumer Control Report Descriptor Template
#define TUD_HID_REPORT_DESC_CONSUMER_CTRL(...) \
  HID_USAGE_PAGE ( HID_USAGE_PAGE_CONSUMER    )              ,\
  HID_USAGE      ( HID_USAGE_CONSUMER_CONTROL )              ,\
  HID_COLLECTION ( HID_COLLECTION_APPLICATION )              ,\
    /* Report ID if any */\
    __VA_ARGS__ \
    HID_LOGICAL_MIN  ( 0x00                                ) ,\
    HID_LOGICAL_MAX_N( 0x0FFF, 2                           ) ,\
    HID_USAGE_MIN    ( 0x00                                ) ,\
    HID_USAGE_MAX_N  ( 0x0FFF, 2                           ) ,\
    HID_REPORT_SIZE  ( 16                                  ) ,\
    HID_REPORT_COUNT ( 2                                   ) ,\
    HID_INPUT        ( HID_DATA | HID_ARRAY | HID_ABSOLUTE ) ,\
  HID_COLLECTION_END \

// System Control Report Descriptor Template
#define TUD_HID_REPORT_DESC_SYSTEM_CTRL(...) \
  HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP    )               ,\
  HID_USAGE      ( HID_USAGE_DESKTOP_SYSTEM_CONTROL )        ,\
  HID_COLLECTION ( HID_COLLECTION_APPLICATION )              ,\
    /* Report ID if any */\
    __VA_ARGS__ \
    HID_LOGICAL_MIN ( 0x00                                )  ,\
    HID_LOGICAL_MAX ( 0xff                                )  ,\
    HID_REPORT_COUNT( 1                                   )  ,\
    HID_REPORT_SIZE ( 8                                   )  ,\
    HID_INPUT        ( HID_DATA | HID_ARRAY | HID_ABSOLUTE ) ,\
  HID_COLLECTION_END \

// Vendor Config Descriptor Template
#define TUD_HID_REPORT_DESC_VENDOR_CTRL(...) \
  HID_USAGE_PAGE_N ( HID_USAGE_PAGE_VENDOR, 2 )             ,\
  HID_USAGE      ( 0x10 )                                   ,\
  HID_COLLECTION ( HID_COLLECTION_APPLICATION )             ,\
    /* Report ID if any */\
    __VA_ARGS__ \
    HID_LOGICAL_MIN ( 0x80                                )  ,\
    HID_LOGICAL_MAX ( 0x7f                                )  ,\
    HID_REPORT_COUNT( 12                                  )  ,\
    HID_REPORT_SIZE ( 8                                   )  ,\
    HID_USAGE       ( 0x10                                )  ,\
    HID_INPUT        ( HID_DATA | HID_ARRAY | HID_ABSOLUTE ) ,\
    HID_USAGE       ( 0x10                                )  ,\
    HID_OUTPUT       ( HID_DATA | HID_ARRAY | HID_ABSOLUTE ) ,\
  HID_COLLECTION_END \

#define HID_USAGE_DIGITIZER 0x01

#define TUD_HID_REPORT_DESC_DIGITIZER_PEN(...) \
HID_USAGE_PAGE ( HID_USAGE_PAGE_DIGITIZER )                 ,\
HID_USAGE ( HID_USAGE_DIGITIZER )                           ,\
HID_COLLECTION ( HID_COLLECTION_APPLICATION )               ,\
  /* Report ID if any */\
  __VA_ARGS__ \
  HID_USAGE ( HID_USAGE_DIGITIZER )                         ,\
  HID_COLLECTION ( HID_COLLECTION_PHYSICAL )                ,\
    HID_USAGE_PAGE ( HID_USAGE_PAGE_DIGITIZER )             ,\
    /* Tip Pressure */\
    HID_USAGE      ( 0x30 )                                 ,\
    HID_LOGICAL_MIN ( 0x00                                )  ,\
    HID_LOGICAL_MAX ( 0xff                                )  ,\
    HID_REPORT_COUNT( 1                                      )  ,\
    HID_REPORT_SIZE ( 8                                      )  ,\
    HID_INPUT ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE )    ,\
    \
    HID_REPORT_COUNT( 5                                      )  ,\
    HID_REPORT_SIZE ( 1                                      )  ,\
    /* In range */\
    HID_USAGE ( 0x32 )                       ,\
    /* Tip switch */\
    HID_USAGE ( 0x42 )                       ,\
    /* Eraser */\
    HID_USAGE ( 0x45 )                       ,\
    /* Barrel switch */\
    HID_USAGE ( 0x44 )                       ,\
    /* Invert */\
    HID_USAGE ( 0x3c )                       ,\
    HID_INPUT ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE )    ,\
    \
    HID_REPORT_COUNT( 3                                      ) ,\
    HID_REPORT_SIZE ( 1                                      ) ,\
    HID_INPUT ( HID_CONSTANT )    ,\
    /* X and Y coordinates */\
    HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP )               ,\
    HID_USAGE ( HID_USAGE_DESKTOP_X )                       ,\
    HID_USAGE ( HID_USAGE_DESKTOP_Y )                       ,\
    HID_LOGICAL_MIN ( 0 )                                   ,\
    HID_LOGICAL_MAX_N ( 32767, 2 )                          ,\
    HID_REPORT_SIZE ( 16 )                                  ,\
    HID_REPORT_COUNT ( 2 )                                  ,\
    HID_INPUT ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE)     ,\
  HID_COLLECTION_END                                        ,\
HID_COLLECTION_END

#endif /* USB_DESCRIPTORS_H_ */
