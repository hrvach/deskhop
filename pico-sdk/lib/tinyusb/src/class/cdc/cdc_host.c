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
 *
 * This file is part of the TinyUSB stack.
 */

#include "tusb_option.h"

#if (CFG_TUH_ENABLED && CFG_TUH_CDC)

#include "host/usbh.h"
#include "host/usbh_pvt.h"

#include "cdc_host.h"

// Level where CFG_TUSB_DEBUG must be at least for this driver is logged
#ifndef CFG_TUH_CDC_LOG_LEVEL
  #define CFG_TUH_CDC_LOG_LEVEL   CFG_TUH_LOG_LEVEL
#endif

#define TU_LOG_DRV(...)   TU_LOG(CFG_TUH_CDC_LOG_LEVEL, __VA_ARGS__)

//--------------------------------------------------------------------+
// Host CDC Interface
//--------------------------------------------------------------------+

typedef struct {
  uint8_t daddr;
  uint8_t bInterfaceNumber;
  uint8_t bInterfaceSubClass;
  uint8_t bInterfaceProtocol;

  uint8_t serial_drid; // Serial Driver ID
  cdc_acm_capability_t acm_capability;
  uint8_t ep_notif;

  uint8_t line_state;                               // DTR (bit0), RTS (bit1)
  TU_ATTR_ALIGNED(4) cdc_line_coding_t line_coding; // Baudrate, stop bits, parity, data width

  tuh_xfer_cb_t user_control_cb;

  struct {
    tu_edpt_stream_t tx;
    tu_edpt_stream_t rx;

    uint8_t tx_ff_buf[CFG_TUH_CDC_TX_BUFSIZE];
    CFG_TUH_MEM_ALIGN uint8_t tx_ep_buf[CFG_TUH_CDC_TX_EPSIZE];

    uint8_t rx_ff_buf[CFG_TUH_CDC_TX_BUFSIZE];
    CFG_TUH_MEM_ALIGN uint8_t rx_ep_buf[CFG_TUH_CDC_TX_EPSIZE];
  } stream;

} cdch_interface_t;

CFG_TUH_MEM_SECTION
static cdch_interface_t cdch_data[CFG_TUH_CDC];

//--------------------------------------------------------------------+
// Serial Driver
//--------------------------------------------------------------------+

//------------- ACM prototypes -------------//
static bool acm_open(uint8_t daddr, tusb_desc_interface_t const *itf_desc, uint16_t max_len);
static void acm_process_config(tuh_xfer_t* xfer);

static bool acm_set_line_coding(cdch_interface_t* p_cdc, cdc_line_coding_t const* line_coding, tuh_xfer_cb_t complete_cb, uintptr_t user_data);
static bool acm_set_control_line_state(cdch_interface_t* p_cdc, uint16_t line_state, tuh_xfer_cb_t complete_cb, uintptr_t user_data);
static bool acm_set_baudrate(cdch_interface_t* p_cdc, uint32_t baudrate, tuh_xfer_cb_t complete_cb, uintptr_t user_data);

//------------- FTDI prototypes -------------//
#if CFG_TUH_CDC_FTDI
#include "serial/ftdi_sio.h"

static uint16_t const ftdi_vid_pid_list[][2] = {CFG_TUH_CDC_FTDI_VID_PID_LIST };
enum {
  FTDI_PID_COUNT = sizeof(ftdi_vid_pid_list) / sizeof(ftdi_vid_pid_list[0])
};

// Store last request baudrate since divisor to baudrate is not easy
static uint32_t _ftdi_requested_baud;

static bool ftdi_open(uint8_t daddr, const tusb_desc_interface_t *itf_desc, uint16_t max_len);
static void ftdi_process_config(tuh_xfer_t* xfer);

static bool ftdi_sio_set_modem_ctrl(cdch_interface_t* p_cdc, uint16_t line_state, tuh_xfer_cb_t complete_cb, uintptr_t user_data);
static bool ftdi_sio_set_baudrate(cdch_interface_t* p_cdc, uint32_t baudrate, tuh_xfer_cb_t complete_cb, uintptr_t user_data);
#endif

//------------- CP210X prototypes -------------//
#if CFG_TUH_CDC_CP210X
#include "serial/cp210x.h"

static uint16_t const cp210x_vid_pid_list[][2] = {CFG_TUH_CDC_CP210X_VID_PID_LIST };
enum {
  CP210X_PID_COUNT = sizeof(cp210x_vid_pid_list) / sizeof(cp210x_vid_pid_list[0])
};

static bool cp210x_open(uint8_t daddr, tusb_desc_interface_t const *itf_desc, uint16_t max_len);
static void cp210x_process_config(tuh_xfer_t* xfer);

static bool cp210x_set_modem_ctrl(cdch_interface_t* p_cdc, uint16_t line_state, tuh_xfer_cb_t complete_cb, uintptr_t user_data);
static bool cp210x_set_baudrate(cdch_interface_t* p_cdc, uint32_t baudrate, tuh_xfer_cb_t complete_cb, uintptr_t user_data);
#endif

enum {
  SERIAL_DRIVER_ACM = 0,

#if CFG_TUH_CDC_FTDI
  SERIAL_DRIVER_FTDI,
#endif

#if CFG_TUH_CDC_CP210X
  SERIAL_DRIVER_CP210X,
#endif
};

typedef struct {
  void (*const process_set_config)(tuh_xfer_t* xfer);
  bool (*const set_control_line_state)(cdch_interface_t* p_cdc, uint16_t line_state, tuh_xfer_cb_t complete_cb, uintptr_t user_data);
  bool (*const set_baudrate)(cdch_interface_t* p_cdc, uint32_t baudrate, tuh_xfer_cb_t complete_cb, uintptr_t user_data);
} cdch_serial_driver_t;

// Note driver list must be in the same order as SERIAL_DRIVER enum
static const cdch_serial_driver_t serial_drivers[] = {
  { .process_set_config     = acm_process_config,
    .set_control_line_state = acm_set_control_line_state,
    .set_baudrate           = acm_set_baudrate
  },

  #if CFG_TUH_CDC_FTDI
  { .process_set_config     = ftdi_process_config,
    .set_control_line_state = ftdi_sio_set_modem_ctrl,
    .set_baudrate           = ftdi_sio_set_baudrate
  },
  #endif

  #if CFG_TUH_CDC_CP210X
  { .process_set_config     = cp210x_process_config,
    .set_control_line_state = cp210x_set_modem_ctrl,
    .set_baudrate           = cp210x_set_baudrate
  },
  #endif
};

enum {
  SERIAL_DRIVER_COUNT = sizeof(serial_drivers) / sizeof(serial_drivers[0])
};

//--------------------------------------------------------------------+
// INTERNAL OBJECT & FUNCTION DECLARATION
//--------------------------------------------------------------------+

static inline cdch_interface_t* get_itf(uint8_t idx)
{
  TU_ASSERT(idx < CFG_TUH_CDC, NULL);
  cdch_interface_t* p_cdc = &cdch_data[idx];

  return (p_cdc->daddr != 0) ? p_cdc : NULL;
}

static inline uint8_t get_idx_by_ep_addr(uint8_t daddr, uint8_t ep_addr)
{
  for(uint8_t i=0; i<CFG_TUH_CDC; i++)
  {
    cdch_interface_t* p_cdc = &cdch_data[i];
    if ( (p_cdc->daddr == daddr) &&
         (ep_addr == p_cdc->ep_notif || ep_addr == p_cdc->stream.rx.ep_addr || ep_addr == p_cdc->stream.tx.ep_addr))
    {
      return i;
    }
  }

  return TUSB_INDEX_INVALID_8;
}


static cdch_interface_t* make_new_itf(uint8_t daddr, tusb_desc_interface_t const *itf_desc)
{
  for(uint8_t i=0; i<CFG_TUH_CDC; i++)
  {
    if (cdch_data[i].daddr == 0) {
      cdch_interface_t* p_cdc = &cdch_data[i];

      p_cdc->daddr              = daddr;
      p_cdc->bInterfaceNumber   = itf_desc->bInterfaceNumber;
      p_cdc->bInterfaceSubClass = itf_desc->bInterfaceSubClass;
      p_cdc->bInterfaceProtocol = itf_desc->bInterfaceProtocol;
      p_cdc->line_state         = 0;
      return p_cdc;
    }
  }

  return NULL;
}

static bool open_ep_stream_pair(cdch_interface_t* p_cdc , tusb_desc_endpoint_t const *desc_ep);
static void set_config_complete(cdch_interface_t * p_cdc, uint8_t idx, uint8_t itf_num);
static void cdch_internal_control_complete(tuh_xfer_t* xfer);

//--------------------------------------------------------------------+
// APPLICATION API
//--------------------------------------------------------------------+

uint8_t tuh_cdc_itf_get_index(uint8_t daddr, uint8_t itf_num)
{
  for(uint8_t i=0; i<CFG_TUH_CDC; i++)
  {
    const cdch_interface_t* p_cdc = &cdch_data[i];

    if (p_cdc->daddr == daddr && p_cdc->bInterfaceNumber == itf_num) return i;
  }

  return TUSB_INDEX_INVALID_8;
}

bool tuh_cdc_itf_get_info(uint8_t idx, tuh_itf_info_t* info)
{
  cdch_interface_t* p_cdc = get_itf(idx);
  TU_VERIFY(p_cdc && info);

  info->daddr = p_cdc->daddr;

  // re-construct descriptor
  tusb_desc_interface_t* desc = &info->desc;
  desc->bLength            = sizeof(tusb_desc_interface_t);
  desc->bDescriptorType    = TUSB_DESC_INTERFACE;

  desc->bInterfaceNumber   = p_cdc->bInterfaceNumber;
  desc->bAlternateSetting  = 0;
  desc->bNumEndpoints      = 2u + (p_cdc->ep_notif ? 1u : 0u);
  desc->bInterfaceClass    = TUSB_CLASS_CDC;
  desc->bInterfaceSubClass = p_cdc->bInterfaceSubClass;
  desc->bInterfaceProtocol = p_cdc->bInterfaceProtocol;
  desc->iInterface         = 0; // not used yet

  return true;
}

bool tuh_cdc_mounted(uint8_t idx)
{
  cdch_interface_t* p_cdc = get_itf(idx);
  return p_cdc != NULL;
}

bool tuh_cdc_get_dtr(uint8_t idx)
{
  cdch_interface_t* p_cdc = get_itf(idx);
  TU_VERIFY(p_cdc);

  return (p_cdc->line_state & CDC_CONTROL_LINE_STATE_DTR) ? true : false;
}

bool tuh_cdc_get_rts(uint8_t idx)
{
  cdch_interface_t* p_cdc = get_itf(idx);
  TU_VERIFY(p_cdc);

  return (p_cdc->line_state & CDC_CONTROL_LINE_STATE_RTS) ? true : false;
}

bool tuh_cdc_get_local_line_coding(uint8_t idx, cdc_line_coding_t* line_coding)
{
  cdch_interface_t* p_cdc = get_itf(idx);
  TU_VERIFY(p_cdc);

  *line_coding = p_cdc->line_coding;

  return true;
}

//--------------------------------------------------------------------+
// Write
//--------------------------------------------------------------------+

uint32_t tuh_cdc_write(uint8_t idx, void const* buffer, uint32_t bufsize)
{
  cdch_interface_t* p_cdc = get_itf(idx);
  TU_VERIFY(p_cdc);

  return tu_edpt_stream_write(&p_cdc->stream.tx, buffer, bufsize);
}

uint32_t tuh_cdc_write_flush(uint8_t idx)
{
  cdch_interface_t* p_cdc = get_itf(idx);
  TU_VERIFY(p_cdc);

  return tu_edpt_stream_write_xfer(&p_cdc->stream.tx);
}

bool tuh_cdc_write_clear(uint8_t idx)
{
  cdch_interface_t* p_cdc = get_itf(idx);
  TU_VERIFY(p_cdc);

  return tu_edpt_stream_clear(&p_cdc->stream.tx);
}

uint32_t tuh_cdc_write_available(uint8_t idx)
{
  cdch_interface_t* p_cdc = get_itf(idx);
  TU_VERIFY(p_cdc);

  return tu_edpt_stream_write_available(&p_cdc->stream.tx);
}

//--------------------------------------------------------------------+
// Read
//--------------------------------------------------------------------+

uint32_t tuh_cdc_read (uint8_t idx, void* buffer, uint32_t bufsize)
{
  cdch_interface_t* p_cdc = get_itf(idx);
  TU_VERIFY(p_cdc);

  return tu_edpt_stream_read(&p_cdc->stream.rx, buffer, bufsize);
}

uint32_t tuh_cdc_read_available(uint8_t idx)
{
  cdch_interface_t* p_cdc = get_itf(idx);
  TU_VERIFY(p_cdc);

  return tu_edpt_stream_read_available(&p_cdc->stream.rx);
}

bool tuh_cdc_peek(uint8_t idx, uint8_t* ch)
{
  cdch_interface_t* p_cdc = get_itf(idx);
  TU_VERIFY(p_cdc);

  return tu_edpt_stream_peek(&p_cdc->stream.rx, ch);
}

bool tuh_cdc_read_clear (uint8_t idx)
{
  cdch_interface_t* p_cdc = get_itf(idx);
  TU_VERIFY(p_cdc);

  bool ret = tu_edpt_stream_clear(&p_cdc->stream.rx);
  tu_edpt_stream_read_xfer(&p_cdc->stream.rx);
  return ret;
}

//--------------------------------------------------------------------+
// Control Endpoint API
//--------------------------------------------------------------------+

// internal control complete to update state such as line state, encoding
static void cdch_internal_control_complete(tuh_xfer_t* xfer)
{
  uint8_t const itf_num = (uint8_t) tu_le16toh(xfer->setup->wIndex);
  uint8_t idx = tuh_cdc_itf_get_index(xfer->daddr, itf_num);
  cdch_interface_t* p_cdc = get_itf(idx);
  TU_ASSERT(p_cdc, );

  if (xfer->result == XFER_RESULT_SUCCESS)
  {
    switch (p_cdc->serial_drid) {
      case SERIAL_DRIVER_ACM:
        switch (xfer->setup->bRequest) {
          case CDC_REQUEST_SET_CONTROL_LINE_STATE:
            p_cdc->line_state = (uint8_t) tu_le16toh(xfer->setup->wValue);
            break;

          case CDC_REQUEST_SET_LINE_CODING: {
            uint16_t const len = tu_min16(sizeof(cdc_line_coding_t), tu_le16toh(xfer->setup->wLength));
            memcpy(&p_cdc->line_coding, xfer->buffer, len);
          }
            break;

          default: break;
        }
        break;

      #if CFG_TUH_CDC_FTDI
      case SERIAL_DRIVER_FTDI:
        switch (xfer->setup->bRequest) {
          case FTDI_SIO_MODEM_CTRL:
            p_cdc->line_state = (uint8_t) (tu_le16toh(xfer->setup->wValue) & 0x00ff);
            break;

          case FTDI_SIO_SET_BAUD_RATE:
            // convert from divisor to baudrate is not supported
            p_cdc->line_coding.bit_rate = _ftdi_requested_baud;
            break;

          default: break;
        }
        break;
      #endif

      #if CFG_TUH_CDC_CP210X
      case SERIAL_DRIVER_CP210X:
        switch(xfer->setup->bRequest) {
          case CP210X_SET_MHS:
            p_cdc->line_state = (uint8_t) (tu_le16toh(xfer->setup->wValue) & 0x00ff);
            break;

          case CP210X_SET_BAUDRATE: {
            uint32_t baudrate;
            memcpy(&baudrate, xfer->buffer, sizeof(uint32_t));
            p_cdc->line_coding.bit_rate = tu_le32toh(baudrate);
          }
            break;
        }
        break;
      #endif

      default: break;
    }
  }

  xfer->complete_cb = p_cdc->user_control_cb;
  if (xfer->complete_cb) {
    xfer->complete_cb(xfer);
  }
}

bool tuh_cdc_set_control_line_state(uint8_t idx, uint16_t line_state, tuh_xfer_cb_t complete_cb, uintptr_t user_data) {
  cdch_interface_t* p_cdc = get_itf(idx);
  TU_VERIFY(p_cdc && p_cdc->serial_drid < SERIAL_DRIVER_COUNT);
  cdch_serial_driver_t const* driver = &serial_drivers[p_cdc->serial_drid];

  if ( complete_cb ) {
    return driver->set_control_line_state(p_cdc, line_state, complete_cb, user_data);
  }else {
    // blocking
    xfer_result_t result = XFER_RESULT_INVALID;
    bool ret = driver->set_control_line_state(p_cdc, line_state, complete_cb, (uintptr_t) &result);

    if (user_data) {
      // user_data is not NULL, return result via user_data
      *((xfer_result_t*) user_data) = result;
    }

    TU_VERIFY(ret && result == XFER_RESULT_SUCCESS);

    p_cdc->line_state = (uint8_t) line_state;
    return true;
  }
}

bool tuh_cdc_set_baudrate(uint8_t idx, uint32_t baudrate, tuh_xfer_cb_t complete_cb, uintptr_t user_data) {
  cdch_interface_t* p_cdc = get_itf(idx);
  TU_VERIFY(p_cdc && p_cdc->serial_drid < SERIAL_DRIVER_COUNT);
  cdch_serial_driver_t const* driver = &serial_drivers[p_cdc->serial_drid];

  if ( complete_cb ) {
    return driver->set_baudrate(p_cdc, baudrate, complete_cb, user_data);
  }else {
    // blocking
    xfer_result_t result = XFER_RESULT_INVALID;
    bool ret = driver->set_baudrate(p_cdc, baudrate, complete_cb, (uintptr_t) &result);

    if (user_data) {
      // user_data is not NULL, return result via user_data
      *((xfer_result_t*) user_data) = result;
    }

    TU_VERIFY(ret && result == XFER_RESULT_SUCCESS);

    p_cdc->line_coding.bit_rate = baudrate;
    return true;
  }
}

bool tuh_cdc_set_line_coding(uint8_t idx, cdc_line_coding_t const* line_coding, tuh_xfer_cb_t complete_cb, uintptr_t user_data)
{
  cdch_interface_t* p_cdc = get_itf(idx);
  // only ACM support this set line coding request
  TU_VERIFY(p_cdc && p_cdc->serial_drid == SERIAL_DRIVER_ACM);
  TU_VERIFY(p_cdc->acm_capability.support_line_request);

  if ( complete_cb ) {
    return acm_set_line_coding(p_cdc, line_coding, complete_cb, user_data);
  }else {
    // blocking
    xfer_result_t result = XFER_RESULT_INVALID;
    bool ret = acm_set_line_coding(p_cdc, line_coding, complete_cb, (uintptr_t) &result);

    if (user_data) {
      // user_data is not NULL, return result via user_data
      *((xfer_result_t*) user_data) = result;
    }

    TU_VERIFY(ret && result == XFER_RESULT_SUCCESS);

    p_cdc->line_coding = *line_coding;
    return true;
  }
}

//--------------------------------------------------------------------+
// CLASS-USBH API
//--------------------------------------------------------------------+

void cdch_init(void)
{
  tu_memclr(cdch_data, sizeof(cdch_data));

  for(size_t i=0; i<CFG_TUH_CDC; i++)
  {
    cdch_interface_t* p_cdc = &cdch_data[i];

    tu_edpt_stream_init(&p_cdc->stream.tx, true, true, false,
                          p_cdc->stream.tx_ff_buf, CFG_TUH_CDC_TX_BUFSIZE,
                          p_cdc->stream.tx_ep_buf, CFG_TUH_CDC_TX_EPSIZE);

    tu_edpt_stream_init(&p_cdc->stream.rx, true, false, false,
                          p_cdc->stream.rx_ff_buf, CFG_TUH_CDC_RX_BUFSIZE,
                          p_cdc->stream.rx_ep_buf, CFG_TUH_CDC_RX_EPSIZE);
  }
}

void cdch_close(uint8_t daddr)
{
  for(uint8_t idx=0; idx<CFG_TUH_CDC; idx++)
  {
    cdch_interface_t* p_cdc = &cdch_data[idx];
    if (p_cdc->daddr == daddr)
    {
      TU_LOG_DRV("  CDCh close addr = %u index = %u\r\n", daddr, idx);

      // Invoke application callback
      if (tuh_cdc_umount_cb) tuh_cdc_umount_cb(idx);

      //tu_memclr(p_cdc, sizeof(cdch_interface_t));
      p_cdc->daddr = 0;
      p_cdc->bInterfaceNumber = 0;
      tu_edpt_stream_close(&p_cdc->stream.tx);
      tu_edpt_stream_close(&p_cdc->stream.rx);
    }
  }
}

bool cdch_xfer_cb(uint8_t daddr, uint8_t ep_addr, xfer_result_t event, uint32_t xferred_bytes) {
  // TODO handle stall response, retry failed transfer ...
  TU_ASSERT(event == XFER_RESULT_SUCCESS);

  uint8_t const idx = get_idx_by_ep_addr(daddr, ep_addr);
  cdch_interface_t * p_cdc = get_itf(idx);
  TU_ASSERT(p_cdc);

  if ( ep_addr == p_cdc->stream.tx.ep_addr ) {
    // invoke tx complete callback to possibly refill tx fifo
    if (tuh_cdc_tx_complete_cb) tuh_cdc_tx_complete_cb(idx);

    if ( 0 == tu_edpt_stream_write_xfer(&p_cdc->stream.tx) ) {
      // If there is no data left, a ZLP should be sent if:
      // - xferred_bytes is multiple of EP Packet size and not zero
      tu_edpt_stream_write_zlp_if_needed(&p_cdc->stream.tx, xferred_bytes);
    }
  }
  else if ( ep_addr == p_cdc->stream.rx.ep_addr ) {
    #if CFG_TUH_CDC_FTDI
    if (p_cdc->serial_drid == SERIAL_DRIVER_FTDI) {
      // FTDI reserve 2 bytes for status
      // FTDI status
//      uint8_t status[2] = {
//        p_cdc->stream.rx.ep_buf[0],
//        p_cdc->stream.rx.ep_buf[1]
//      };
      tu_edpt_stream_read_xfer_complete_offset(&p_cdc->stream.rx, xferred_bytes, 2);
    }else
    #endif
    {
      tu_edpt_stream_read_xfer_complete(&p_cdc->stream.rx, xferred_bytes);
    }

    // invoke receive callback
    if (tuh_cdc_rx_cb) tuh_cdc_rx_cb(idx);

    // prepare for next transfer if needed
    tu_edpt_stream_read_xfer(&p_cdc->stream.rx);
  }else if ( ep_addr == p_cdc->ep_notif ) {
    // TODO handle notification endpoint
  }else {
    TU_ASSERT(false);
  }

  return true;
}

//--------------------------------------------------------------------+
// Enumeration
//--------------------------------------------------------------------+

static bool open_ep_stream_pair(cdch_interface_t* p_cdc, tusb_desc_endpoint_t const *desc_ep)
{
  for(size_t i=0; i<2; i++)
  {
    TU_ASSERT(TUSB_DESC_ENDPOINT == desc_ep->bDescriptorType &&
              TUSB_XFER_BULK     == desc_ep->bmAttributes.xfer);

    TU_ASSERT(tuh_edpt_open(p_cdc->daddr, desc_ep));

    if ( tu_edpt_dir(desc_ep->bEndpointAddress) == TUSB_DIR_IN )
    {
      tu_edpt_stream_open(&p_cdc->stream.rx, p_cdc->daddr, desc_ep);
    }else
    {
      tu_edpt_stream_open(&p_cdc->stream.tx, p_cdc->daddr, desc_ep);
    }

    desc_ep = (tusb_desc_endpoint_t const*) tu_desc_next(desc_ep);
  }

  return true;
}

bool cdch_open(uint8_t rhport, uint8_t daddr, tusb_desc_interface_t const *itf_desc, uint16_t max_len)
{
  (void) rhport;

  // Only support ACM subclass
  // Note: Protocol 0xFF can be RNDIS device
  if ( TUSB_CLASS_CDC                           == itf_desc->bInterfaceClass &&
       CDC_COMM_SUBCLASS_ABSTRACT_CONTROL_MODEL == itf_desc->bInterfaceSubClass)
  {
    return acm_open(daddr, itf_desc, max_len);
  }
  #if CFG_TUH_CDC_FTDI || CFG_TUH_CDC_CP210X
  else if ( TUSB_CLASS_VENDOR_SPECIFIC == itf_desc->bInterfaceClass )
  {
    uint16_t vid, pid;
    TU_VERIFY(tuh_vid_pid_get(daddr, &vid, &pid));

    #if CFG_TUH_CDC_FTDI
    for (size_t i = 0; i < FTDI_PID_COUNT; i++) {
      if (ftdi_vid_pid_list[i][0] == vid && ftdi_vid_pid_list[i][1] == pid) {
        return ftdi_open(daddr, itf_desc, max_len);
      }
    }
    #endif

    #if CFG_TUH_CDC_CP210X
    for (size_t i = 0; i < CP210X_PID_COUNT; i++) {
      if (cp210x_vid_pid_list[i][0] == vid && cp210x_vid_pid_list[i][1] == pid) {
        return cp210x_open(daddr, itf_desc, max_len);
      }
    }
    #endif
  }
  #endif

  return false;
}

static void set_config_complete(cdch_interface_t * p_cdc, uint8_t idx, uint8_t itf_num) {
  if (tuh_cdc_mount_cb) tuh_cdc_mount_cb(idx);

  // Prepare for incoming data
  tu_edpt_stream_read_xfer(&p_cdc->stream.rx);

  // notify usbh that driver enumeration is complete
  usbh_driver_set_config_complete(p_cdc->daddr, itf_num);
}


bool cdch_set_config(uint8_t daddr, uint8_t itf_num)
{
  tusb_control_request_t request;
  request.wIndex = tu_htole16((uint16_t) itf_num);

  // fake transfer to kick-off process
  tuh_xfer_t xfer;
  xfer.daddr  = daddr;
  xfer.result = XFER_RESULT_SUCCESS;
  xfer.setup  = &request;
  xfer.user_data = 0; // initial state

  uint8_t const idx = tuh_cdc_itf_get_index(daddr, itf_num);
  cdch_interface_t * p_cdc = get_itf(idx);
  TU_ASSERT(p_cdc && p_cdc->serial_drid < SERIAL_DRIVER_COUNT);

  serial_drivers[p_cdc->serial_drid].process_set_config(&xfer);
  return true;
}

//--------------------------------------------------------------------+
// ACM
//--------------------------------------------------------------------+

enum {
  CONFIG_ACM_SET_CONTROL_LINE_STATE = 0,
  CONFIG_ACM_SET_LINE_CODING,
  CONFIG_ACM_COMPLETE,
};

static bool acm_open(uint8_t daddr, tusb_desc_interface_t const *itf_desc, uint16_t max_len)
{
  uint8_t const * p_desc_end = ((uint8_t const*) itf_desc) + max_len;

  cdch_interface_t * p_cdc = make_new_itf(daddr, itf_desc);
  TU_VERIFY(p_cdc);

  p_cdc->serial_drid = SERIAL_DRIVER_ACM;

  //------------- Control Interface -------------//
  uint8_t const * p_desc = tu_desc_next(itf_desc);

  // Communication Functional Descriptors
  while( (p_desc < p_desc_end) && (TUSB_DESC_CS_INTERFACE == tu_desc_type(p_desc)) )
  {
    if ( CDC_FUNC_DESC_ABSTRACT_CONTROL_MANAGEMENT == cdc_functional_desc_typeof(p_desc) )
    {
      // save ACM bmCapabilities
      p_cdc->acm_capability = ((cdc_desc_func_acm_t const *) p_desc)->bmCapabilities;
    }

    p_desc = tu_desc_next(p_desc);
  }

  // Open notification endpoint of control interface if any
  if (itf_desc->bNumEndpoints == 1)
  {
    TU_ASSERT(TUSB_DESC_ENDPOINT == tu_desc_type(p_desc));
    tusb_desc_endpoint_t const * desc_ep = (tusb_desc_endpoint_t const *) p_desc;

    TU_ASSERT( tuh_edpt_open(daddr, desc_ep) );
    p_cdc->ep_notif = desc_ep->bEndpointAddress;

    p_desc = tu_desc_next(p_desc);
  }

  //------------- Data Interface (if any) -------------//
  if ( (TUSB_DESC_INTERFACE == tu_desc_type(p_desc)) &&
       (TUSB_CLASS_CDC_DATA == ((tusb_desc_interface_t const *) p_desc)->bInterfaceClass) )
  {
    // next to endpoint descriptor
    p_desc = tu_desc_next(p_desc);

    // data endpoints expected to be in pairs
    TU_ASSERT(open_ep_stream_pair(p_cdc, (tusb_desc_endpoint_t const *) p_desc));
  }

  return true;
}

static void acm_process_config(tuh_xfer_t* xfer)
{
  uintptr_t const state = xfer->user_data;
  uint8_t const itf_num = (uint8_t) tu_le16toh(xfer->setup->wIndex);
  uint8_t const idx = tuh_cdc_itf_get_index(xfer->daddr, itf_num);
  cdch_interface_t * p_cdc = get_itf(idx);
  TU_ASSERT(p_cdc, );

  switch(state)
  {
    case CONFIG_ACM_SET_CONTROL_LINE_STATE:
      #if CFG_TUH_CDC_LINE_CONTROL_ON_ENUM
      if (p_cdc->acm_capability.support_line_request)
      {
        TU_ASSERT(acm_set_control_line_state(p_cdc, CFG_TUH_CDC_LINE_CONTROL_ON_ENUM, acm_process_config,
                                             CONFIG_ACM_SET_LINE_CODING), );
        break;
      }
          #endif
      TU_ATTR_FALLTHROUGH;

    case CONFIG_ACM_SET_LINE_CODING:
        #ifdef CFG_TUH_CDC_LINE_CODING_ON_ENUM
      if (p_cdc->acm_capability.support_line_request)
      {
        cdc_line_coding_t line_coding = CFG_TUH_CDC_LINE_CODING_ON_ENUM;
        TU_ASSERT(acm_set_line_coding(p_cdc, &line_coding, acm_process_config, CONFIG_ACM_COMPLETE), );
        break;
      }
        #endif
      TU_ATTR_FALLTHROUGH;

    case CONFIG_ACM_COMPLETE:
      // itf_num+1 to account for data interface as well
      set_config_complete(p_cdc, idx, itf_num+1);
      break;

    default: break;
  }
}

static bool acm_set_control_line_state(cdch_interface_t* p_cdc, uint16_t line_state, tuh_xfer_cb_t complete_cb, uintptr_t user_data) {
  TU_VERIFY(p_cdc->acm_capability.support_line_request);
  TU_LOG_DRV("CDC ACM Set Control Line State\r\n");

  tusb_control_request_t const request = {
    .bmRequestType_bit = {
      .recipient = TUSB_REQ_RCPT_INTERFACE,
      .type      = TUSB_REQ_TYPE_CLASS,
      .direction = TUSB_DIR_OUT
    },
    .bRequest = CDC_REQUEST_SET_CONTROL_LINE_STATE,
    .wValue   = tu_htole16(line_state),
    .wIndex   = tu_htole16((uint16_t) p_cdc->bInterfaceNumber),
    .wLength  = 0
  };

  p_cdc->user_control_cb = complete_cb;

  tuh_xfer_t xfer = {
    .daddr       = p_cdc->daddr,
    .ep_addr     = 0,
    .setup       = &request,
    .buffer      = NULL,
    .complete_cb = complete_cb ? cdch_internal_control_complete : NULL, // complete_cb is NULL for sync call
    .user_data   = user_data
  };

  TU_ASSERT(tuh_control_xfer(&xfer));
  return true;
}

static bool acm_set_line_coding(cdch_interface_t* p_cdc, cdc_line_coding_t const* line_coding, tuh_xfer_cb_t complete_cb, uintptr_t user_data) {
  TU_LOG_DRV("CDC ACM Set Line Conding\r\n");

  tusb_control_request_t const request = {
    .bmRequestType_bit = {
      .recipient = TUSB_REQ_RCPT_INTERFACE,
      .type      = TUSB_REQ_TYPE_CLASS,
      .direction = TUSB_DIR_OUT
    },
    .bRequest = CDC_REQUEST_SET_LINE_CODING,
    .wValue   = 0,
    .wIndex   = tu_htole16(p_cdc->bInterfaceNumber),
    .wLength  = tu_htole16(sizeof(cdc_line_coding_t))
  };

  // use usbh enum buf to hold line coding since user line_coding variable does not live long enough
  uint8_t* enum_buf = usbh_get_enum_buf();
  memcpy(enum_buf, line_coding, sizeof(cdc_line_coding_t));

  p_cdc->user_control_cb = complete_cb;
  tuh_xfer_t xfer = {
    .daddr       = p_cdc->daddr,
    .ep_addr     = 0,
    .setup       = &request,
    .buffer      = enum_buf,
    .complete_cb = complete_cb ? cdch_internal_control_complete : NULL, // complete_cb is NULL for sync call
    .user_data   = user_data
  };

  TU_ASSERT(tuh_control_xfer(&xfer));
  return true;
}

static bool acm_set_baudrate(cdch_interface_t* p_cdc, uint32_t baudrate, tuh_xfer_cb_t complete_cb, uintptr_t user_data) {
  TU_VERIFY(p_cdc->acm_capability.support_line_request);
  cdc_line_coding_t line_coding = p_cdc->line_coding;
  line_coding.bit_rate = baudrate;
  return acm_set_line_coding(p_cdc, &line_coding, complete_cb, user_data);
}

//--------------------------------------------------------------------+
// FTDI
//--------------------------------------------------------------------+
#if CFG_TUH_CDC_FTDI

enum {
  CONFIG_FTDI_RESET = 0,
  CONFIG_FTDI_MODEM_CTRL,
  CONFIG_FTDI_SET_BAUDRATE,
  CONFIG_FTDI_SET_DATA,
  CONFIG_FTDI_COMPLETE
};

static bool ftdi_open(uint8_t daddr, const tusb_desc_interface_t *itf_desc, uint16_t max_len) {
  // FTDI Interface includes 1 vendor interface + 2 bulk endpoints
  TU_VERIFY(itf_desc->bInterfaceSubClass == 0xff && itf_desc->bInterfaceProtocol == 0xff && itf_desc->bNumEndpoints == 2);
  TU_VERIFY(sizeof(tusb_desc_interface_t) + 2*sizeof(tusb_desc_endpoint_t) <= max_len);

  cdch_interface_t * p_cdc = make_new_itf(daddr, itf_desc);
  TU_VERIFY(p_cdc);

  TU_LOG_DRV("FTDI opened\r\n");

  p_cdc->serial_drid = SERIAL_DRIVER_FTDI;

  // endpoint pair
  tusb_desc_endpoint_t const * desc_ep = (tusb_desc_endpoint_t const *) tu_desc_next(itf_desc);

  // data endpoints expected to be in pairs
  return open_ep_stream_pair(p_cdc, desc_ep);
}

// set request without data
static bool ftdi_sio_set_request(cdch_interface_t* p_cdc, uint8_t command, uint16_t value, tuh_xfer_cb_t complete_cb, uintptr_t user_data) {
  tusb_control_request_t const request = {
    .bmRequestType_bit = {
      .recipient = TUSB_REQ_RCPT_DEVICE,
      .type      = TUSB_REQ_TYPE_VENDOR,
      .direction = TUSB_DIR_OUT
    },
    .bRequest = command,
    .wValue   = tu_htole16(value),
    .wIndex   = 0,
    .wLength  = 0
  };

  tuh_xfer_t xfer = {
    .daddr       = p_cdc->daddr,
    .ep_addr     = 0,
    .setup       = &request,
    .buffer      = NULL,
    .complete_cb = complete_cb,
    .user_data   = user_data
  };

  return tuh_control_xfer(&xfer);
}

static bool ftdi_sio_reset(cdch_interface_t* p_cdc, tuh_xfer_cb_t complete_cb, uintptr_t user_data)
{
  return ftdi_sio_set_request(p_cdc, FTDI_SIO_RESET, FTDI_SIO_RESET_SIO, complete_cb, user_data);
}

static bool ftdi_sio_set_modem_ctrl(cdch_interface_t* p_cdc, uint16_t line_state, tuh_xfer_cb_t complete_cb, uintptr_t user_data)
{
  TU_LOG_DRV("CDC FTDI Set Control Line State\r\n");
  p_cdc->user_control_cb = complete_cb;
  TU_ASSERT(ftdi_sio_set_request(p_cdc, FTDI_SIO_MODEM_CTRL, 0x0300 | line_state,
                                 complete_cb ? cdch_internal_control_complete : NULL, user_data));
  return true;
}

static uint32_t ftdi_232bm_baud_base_to_divisor(uint32_t baud, uint32_t base)
{
  const uint8_t divfrac[8] = { 0, 3, 2, 4, 1, 5, 6, 7 };
  uint32_t divisor;

  /* divisor shifted 3 bits to the left */
  uint32_t divisor3 = base / (2 * baud);
  divisor = (divisor3 >> 3);
  divisor |= (uint32_t) divfrac[divisor3 & 0x7] << 14;

  /* Deal with special cases for highest baud rates. */
  if (divisor == 1) { /* 1.0 */
    divisor = 0;
  }
  else if (divisor == 0x4001) { /* 1.5 */
    divisor = 1;
  }

  return divisor;
}

static uint32_t ftdi_232bm_baud_to_divisor(uint32_t baud)
{
  return ftdi_232bm_baud_base_to_divisor(baud, 48000000u);
}

static bool ftdi_sio_set_baudrate(cdch_interface_t* p_cdc, uint32_t baudrate, tuh_xfer_cb_t complete_cb, uintptr_t user_data)
{
  uint16_t const divisor = (uint16_t) ftdi_232bm_baud_to_divisor(baudrate);
  TU_LOG_DRV("CDC FTDI Set BaudRate = %lu, divisor = 0x%04x\r\n", baudrate, divisor);

  p_cdc->user_control_cb = complete_cb;
  _ftdi_requested_baud = baudrate;
  TU_ASSERT(ftdi_sio_set_request(p_cdc, FTDI_SIO_SET_BAUD_RATE, divisor,
                                 complete_cb ? cdch_internal_control_complete : NULL, user_data));

  return true;
}

static void ftdi_process_config(tuh_xfer_t* xfer) {
  uintptr_t const state = xfer->user_data;
  uint8_t const itf_num = (uint8_t) tu_le16toh(xfer->setup->wIndex);
  uint8_t const idx = tuh_cdc_itf_get_index(xfer->daddr, itf_num);
  cdch_interface_t * p_cdc = get_itf(idx);
  TU_ASSERT(p_cdc, );

  switch(state) {
    // Note may need to read FTDI eeprom
    case CONFIG_FTDI_RESET:
      TU_ASSERT(ftdi_sio_reset(p_cdc, ftdi_process_config, CONFIG_FTDI_MODEM_CTRL),);
      break;

    case CONFIG_FTDI_MODEM_CTRL:
      #if CFG_TUH_CDC_LINE_CONTROL_ON_ENUM
      TU_ASSERT(
        ftdi_sio_set_modem_ctrl(p_cdc, CFG_TUH_CDC_LINE_CONTROL_ON_ENUM, ftdi_process_config, CONFIG_FTDI_SET_BAUDRATE),);
      break;
      #else
      TU_ATTR_FALLTHROUGH;
      #endif

    case CONFIG_FTDI_SET_BAUDRATE: {
      #ifdef CFG_TUH_CDC_LINE_CODING_ON_ENUM
      cdc_line_coding_t line_coding = CFG_TUH_CDC_LINE_CODING_ON_ENUM;
      TU_ASSERT(ftdi_sio_set_baudrate(p_cdc, line_coding.bit_rate, ftdi_process_config, CONFIG_FTDI_SET_DATA),);
      break;
      #else
      TU_ATTR_FALLTHROUGH;
      #endif
    }

    case CONFIG_FTDI_SET_DATA: {
      #if 0 // TODO set data format
      #ifdef CFG_TUH_CDC_LINE_CODING_ON_ENUM
      cdc_line_coding_t line_coding = CFG_TUH_CDC_LINE_CODING_ON_ENUM;
      TU_ASSERT(ftdi_sio_set_data(p_cdc, process_ftdi_config, CONFIG_FTDI_COMPLETE),);
      break;
      #endif
      #endif

      TU_ATTR_FALLTHROUGH;
    }

    case CONFIG_FTDI_COMPLETE:
      set_config_complete(p_cdc, idx, itf_num);
      break;

    default:
      break;
  }
}

#endif

//--------------------------------------------------------------------+
// CP210x
//--------------------------------------------------------------------+

#if CFG_TUH_CDC_CP210X

enum {
  CONFIG_CP210X_IFC_ENABLE = 0,
  CONFIG_CP210X_SET_BAUDRATE,
  CONFIG_CP210X_SET_LINE_CTL,
  CONFIG_CP210X_SET_DTR_RTS,
  CONFIG_CP210X_COMPLETE
};

static bool cp210x_open(uint8_t daddr, tusb_desc_interface_t const *itf_desc, uint16_t max_len) {
  // CP210x Interface includes 1 vendor interface + 2 bulk endpoints
  TU_VERIFY(itf_desc->bInterfaceSubClass == 0 && itf_desc->bInterfaceProtocol == 0 && itf_desc->bNumEndpoints == 2);
  TU_VERIFY(sizeof(tusb_desc_interface_t) + 2*sizeof(tusb_desc_endpoint_t) <= max_len);

  cdch_interface_t * p_cdc = make_new_itf(daddr, itf_desc);
  TU_VERIFY(p_cdc);

  TU_LOG_DRV("CP210x opened\r\n");
  p_cdc->serial_drid = SERIAL_DRIVER_CP210X;

  // endpoint pair
  tusb_desc_endpoint_t const * desc_ep = (tusb_desc_endpoint_t const *) tu_desc_next(itf_desc);

  // data endpoints expected to be in pairs
  return open_ep_stream_pair(p_cdc, desc_ep);
}

static bool cp210x_set_request(cdch_interface_t* p_cdc, uint8_t command, uint16_t value, uint8_t* buffer, uint16_t length, tuh_xfer_cb_t complete_cb, uintptr_t user_data) {
  tusb_control_request_t const request = {
    .bmRequestType_bit = {
      .recipient = TUSB_REQ_RCPT_INTERFACE,
      .type      = TUSB_REQ_TYPE_VENDOR,
      .direction = TUSB_DIR_OUT
    },
    .bRequest = command,
    .wValue   = tu_htole16(value),
    .wIndex   = p_cdc->bInterfaceNumber,
    .wLength  = tu_htole16(length)
  };

  // use usbh enum buf since application variable does not live long enough
  uint8_t* enum_buf = NULL;

  if (buffer && length > 0) {
    enum_buf = usbh_get_enum_buf();
    tu_memcpy_s(enum_buf, CFG_TUH_ENUMERATION_BUFSIZE, buffer, length);
  }

  tuh_xfer_t xfer = {
    .daddr       = p_cdc->daddr,
    .ep_addr     = 0,
    .setup       = &request,
    .buffer      = enum_buf,
    .complete_cb = complete_cb,
    .user_data   = user_data
  };

  return tuh_control_xfer(&xfer);
}

static bool cp210x_ifc_enable(cdch_interface_t* p_cdc, uint16_t enabled, tuh_xfer_cb_t complete_cb, uintptr_t user_data) {
  return cp210x_set_request(p_cdc, CP210X_IFC_ENABLE, enabled, NULL, 0, complete_cb, user_data);
}

static bool cp210x_set_baudrate(cdch_interface_t* p_cdc, uint32_t baudrate, tuh_xfer_cb_t complete_cb, uintptr_t user_data) {
  TU_LOG_DRV("CDC CP210x Set BaudRate = %lu\r\n", baudrate);
  uint32_t baud_le = tu_htole32(baudrate);
  p_cdc->user_control_cb = complete_cb;
  return cp210x_set_request(p_cdc, CP210X_SET_BAUDRATE, 0, (uint8_t *) &baud_le, 4,
                            complete_cb ? cdch_internal_control_complete : NULL, user_data);
}

static bool cp210x_set_modem_ctrl(cdch_interface_t* p_cdc, uint16_t line_state, tuh_xfer_cb_t complete_cb, uintptr_t user_data)
{
  TU_LOG_DRV("CDC CP210x Set Control Line State\r\n");
  p_cdc->user_control_cb = complete_cb;
  return cp210x_set_request(p_cdc, CP210X_SET_MHS, 0x0300 | line_state, NULL, 0,
                            complete_cb ? cdch_internal_control_complete : NULL, user_data);
}

static void cp210x_process_config(tuh_xfer_t* xfer) {
  uintptr_t const state   = xfer->user_data;
  uint8_t const   itf_num = (uint8_t) tu_le16toh(xfer->setup->wIndex);
  uint8_t const   idx     = tuh_cdc_itf_get_index(xfer->daddr, itf_num);
  cdch_interface_t *p_cdc = get_itf(idx);
  TU_ASSERT(p_cdc,);

  switch (state) {
    case CONFIG_CP210X_IFC_ENABLE:
      TU_ASSERT(cp210x_ifc_enable(p_cdc, 1, cp210x_process_config, CONFIG_CP210X_SET_BAUDRATE),);
      break;

    case CONFIG_CP210X_SET_BAUDRATE: {
      #ifdef CFG_TUH_CDC_LINE_CODING_ON_ENUM
      cdc_line_coding_t line_coding = CFG_TUH_CDC_LINE_CODING_ON_ENUM;
      TU_ASSERT(cp210x_set_baudrate(p_cdc, line_coding.bit_rate, cp210x_process_config, CONFIG_CP210X_SET_LINE_CTL),);
      break;
      #else
      TU_ATTR_FALLTHROUGH;
      #endif
    }

    case CONFIG_CP210X_SET_LINE_CTL: {
      #if defined(CFG_TUH_CDC_LINE_CODING_ON_ENUM) && 0 // skip for now
      cdc_line_coding_t line_coding = CFG_TUH_CDC_LINE_CODING_ON_ENUM;
      break;
      #else
      TU_ATTR_FALLTHROUGH;
      #endif
    }

    case CONFIG_CP210X_SET_DTR_RTS:
      #if CFG_TUH_CDC_LINE_CONTROL_ON_ENUM
      TU_ASSERT(
        cp210x_set_modem_ctrl(p_cdc, CFG_TUH_CDC_LINE_CONTROL_ON_ENUM, cp210x_process_config, CONFIG_CP210X_COMPLETE),);
      break;
      #else
      TU_ATTR_FALLTHROUGH;
      #endif

    case CONFIG_CP210X_COMPLETE:
      set_config_complete(p_cdc, idx, itf_num);
      break;

    default: break;
  }
}

#endif

#endif
