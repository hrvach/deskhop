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

enum
{
  REPORT_ID_KEYBOARD = 1,
  REPORT_ID_MOUSE,
  REPORT_ID_CONSUMER_CONTROL,
  REPORT_ID_COUNT
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
    HID_USAGE_PAGE  ( HID_USAGE_PAGE_CONSUMER ), \
      \
      /* Horizontal wheel scroll [-127, 127] */ \
      HID_USAGE_N     ( HID_USAGE_CONSUMER_AC_PAN, 2           ), \
      HID_LOGICAL_MIN ( 0x81                                   ), \
      HID_LOGICAL_MAX ( 0x7f                                   ), \
      HID_REPORT_COUNT( 1                                      ), \
      HID_REPORT_SIZE ( 8                                      ), \
      HID_INPUT       ( HID_DATA | HID_VARIABLE | HID_RELATIVE ), \
  HID_COLLECTION_END                                            , \
HID_COLLECTION_END \

/*                      Generated report                        */
/*                                                              */
/*    0x05, 0x01,           Usage Page (Desktop),               */
/*    0x09, 0x06,           Usage (Keyboard),                   */
/*    0xA1, 0x01,           Collection (Application),           */
/*    0x85, 0x01,               Report ID (1),                  */
/*    0x05, 0x07,               Usage Page (Keyboard),          */
/*    0x19, 0xE0,               Usage Minimum (KB Leftcontrol), */
/*    0x29, 0xE7,               Usage Maximum (KB Right GUI),   */
/*    0x15, 0x00,               Logical Minimum (0),            */
/*    0x25, 0x01,               Logical Maximum (1),            */
/*    0x95, 0x08,               Report Count (8),               */
/*    0x75, 0x01,               Report Size (1),                */
/*    0x81, 0x02,               Input (Variable),               */
/*    0x95, 0x01,               Report Count (1),               */
/*    0x75, 0x08,               Report Size (8),                */
/*    0x81, 0x01,               Input (Constant),               */
/*    0x05, 0x08,               Usage Page (LED),               */
/*    0x19, 0x01,               Usage Minimum (01h),            */
/*    0x29, 0x05,               Usage Maximum (05h),            */
/*    0x95, 0x05,               Report Count (5),               */
/*    0x75, 0x01,               Report Size (1),                */
/*    0x91, 0x02,               Output (Variable),              */
/*    0x95, 0x01,               Report Count (1),               */
/*    0x75, 0x03,               Report Size (3),                */
/*    0x91, 0x01,               Output (Constant),              */
/*    0x05, 0x07,               Usage Page (Keyboard),          */
/*    0x19, 0x00,               Usage Minimum (None),           */
/*    0x2A, 0xFF, 0x00,         Usage Maximum (FFh),            */
/*    0x15, 0x00,               Logical Minimum (0),            */
/*    0x26, 0xFF, 0x00,         Logical Maximum (255),          */
/*    0x95, 0x06,               Report Count (6),               */
/*    0x75, 0x08,               Report Size (8),                */
/*    0x81, 0x00,               Input,                          */
/*    0xC0,                 End Collection,                     */
/*    0x05, 0x01,           Usage Page (Desktop),               */
/*    0x09, 0x02,           Usage (Mouse),                      */
/*    0xA1, 0x01,           Collection (Application),           */
/*    0x85, 0x02,               Report ID (2),                  */
/*    0x09, 0x01,               Usage (Pointer),                */
/*    0xA1, 0x00,               Collection (Physical),          */
/*    0x05, 0x09,                   Usage Page (Button),        */
/*    0x19, 0x01,                   Usage Minimum (01h),        */
/*    0x29, 0x05,                   Usage Maximum (05h),        */
/*    0x15, 0x00,                   Logical Minimum (0),        */
/*    0x25, 0x01,                   Logical Maximum (1),        */
/*    0x95, 0x05,                   Report Count (5),           */
/*    0x75, 0x01,                   Report Size (1),            */
/*    0x81, 0x02,                   Input (Variable),           */
/*    0x95, 0x01,                   Report Count (1),           */
/*    0x75, 0x03,                   Report Size (3),            */
/*    0x81, 0x01,                   Input (Constant),           */
/*    0x05, 0x01,                   Usage Page (Desktop),       */
/*    0x09, 0x30,                   Usage (X),                  */
/*    0x09, 0x31,                   Usage (Y),                  */
/*    0x15, 0x00,                   Logical Minimum (0),        */
/*    0x26, 0xFF, 0x7F,             Logical Maximum (32767),    */
/*    0x75, 0x10,                   Report Size (16),           */
/*    0x95, 0x02,                   Report Count (2),           */
/*    0x81, 0x02,                   Input (Variable),           */
/*    0x09, 0x38,                   Usage (Wheel),              */
/*    0x15, 0x81,                   Logical Minimum (-127),     */
/*    0x25, 0x7F,                   Logical Maximum (127),      */
/*    0x95, 0x01,                   Report Count (1),           */
/*    0x75, 0x08,                   Report Size (8),            */
/*    0x81, 0x06,                   Input (Variable, Relative), */
/*    0x05, 0x0C,                   Usage Page (Consumer),      */
/*    0x0A, 0x38, 0x02,             Usage (AC Pan),             */
/*    0x15, 0x81,                   Logical Minimum (-127),     */
/*    0x25, 0x7F,                   Logical Maximum (127),      */
/*    0x95, 0x01,                   Report Count (1),           */
/*    0x75, 0x08,                   Report Size (8),            */
/*    0x81, 0x06,                   Input (Variable, Relative), */
/*    0xC0,                     End Collection,                 */
/*    0xC0                  End Collection                      */

#endif /* USB_DESCRIPTORS_H_ */
