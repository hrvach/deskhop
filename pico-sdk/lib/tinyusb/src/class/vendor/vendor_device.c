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

#if (CFG_TUD_ENABLED && CFG_TUD_VENDOR)

#include "device/usbd.h"
#include "device/usbd_pvt.h"

#include "vendor_device.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF
//--------------------------------------------------------------------+
typedef struct
{
  uint8_t itf_num;
  uint8_t ep_in;
  uint8_t ep_out;

  /*------------- From this point, data is not cleared by bus reset -------------*/
  tu_fifo_t rx_ff;
  tu_fifo_t tx_ff;

  uint8_t rx_ff_buf[CFG_TUD_VENDOR_RX_BUFSIZE];
  uint8_t tx_ff_buf[CFG_TUD_VENDOR_TX_BUFSIZE];

#if CFG_FIFO_MUTEX
  osal_mutex_def_t rx_ff_mutex;
  osal_mutex_def_t tx_ff_mutex;
#endif

  // Endpoint Transfer buffer
  CFG_TUSB_MEM_ALIGN uint8_t epout_buf[CFG_TUD_VENDOR_EPSIZE];
  CFG_TUSB_MEM_ALIGN uint8_t epin_buf[CFG_TUD_VENDOR_EPSIZE];
} vendord_interface_t;

CFG_TUD_MEM_SECTION tu_static vendord_interface_t _vendord_itf[CFG_TUD_VENDOR];

#define ITF_MEM_RESET_SIZE   offsetof(vendord_interface_t, rx_ff)


bool tud_vendor_n_mounted (uint8_t itf)
{
  return _vendord_itf[itf].ep_in && _vendord_itf[itf].ep_out;
}

uint32_t tud_vendor_n_available (uint8_t itf)
{
  return tu_fifo_count(&_vendord_itf[itf].rx_ff);
}

bool tud_vendor_n_peek(uint8_t itf, uint8_t* u8)
{
  return tu_fifo_peek(&_vendord_itf[itf].rx_ff, u8);
}

//--------------------------------------------------------------------+
// Read API
//--------------------------------------------------------------------+
static void _prep_out_transaction (vendord_interface_t* p_itf)
{
  uint8_t const rhport = 0;

    // claim endpoint
  TU_VERIFY(usbd_edpt_claim(rhport, p_itf->ep_out), );

  // Prepare for incoming data but only allow what we can store in the ring buffer.
  uint16_t max_read = tu_fifo_remaining(&p_itf->rx_ff);
  if ( max_read >= CFG_TUD_VENDOR_EPSIZE )
  {
    usbd_edpt_xfer(rhport, p_itf->ep_out, p_itf->epout_buf, CFG_TUD_VENDOR_EPSIZE);
  }
  else
  {
    // Release endpoint since we don't make any transfer
    usbd_edpt_release(rhport, p_itf->ep_out);
  }
}

uint32_t tud_vendor_n_read (uint8_t itf, void* buffer, uint32_t bufsize)
{
  vendord_interface_t* p_itf = &_vendord_itf[itf];
  uint32_t num_read = tu_fifo_read_n(&p_itf->rx_ff, buffer, (uint16_t) bufsize);
  _prep_out_transaction(p_itf);
  return num_read;
}

void tud_vendor_n_read_flush (uint8_t itf)
{
  vendord_interface_t* p_itf = &_vendord_itf[itf];
  tu_fifo_clear(&p_itf->rx_ff);
  _prep_out_transaction(p_itf);
}

//--------------------------------------------------------------------+
// Write API
//--------------------------------------------------------------------+
uint32_t tud_vendor_n_write (uint8_t itf, void const* buffer, uint32_t bufsize)
{
  vendord_interface_t* p_itf = &_vendord_itf[itf];
  uint16_t ret = tu_fifo_write_n(&p_itf->tx_ff, buffer, (uint16_t) bufsize);

  // flush if queue more than packet size
  if (tu_fifo_count(&p_itf->tx_ff) >= CFG_TUD_VENDOR_EPSIZE) {
    tud_vendor_n_write_flush(itf);
  }
  return ret;
}

uint32_t tud_vendor_n_write_flush (uint8_t itf)
{
  vendord_interface_t* p_itf = &_vendord_itf[itf];

  // Skip if usb is not ready yet
  TU_VERIFY( tud_ready(), 0 );

  // No data to send
  if ( !tu_fifo_count(&p_itf->tx_ff) ) return 0;

  uint8_t const rhport = 0;

  // Claim the endpoint
  TU_VERIFY( usbd_edpt_claim(rhport, p_itf->ep_in), 0 );

  // Pull data from FIFO
  uint16_t const count = tu_fifo_read_n(&p_itf->tx_ff, p_itf->epin_buf, sizeof(p_itf->epin_buf));

  if ( count )
  {
    TU_ASSERT( usbd_edpt_xfer(rhport, p_itf->ep_in, p_itf->epin_buf, count), 0 );
    return count;
  }else
  {
    // Release endpoint since we don't make any transfer
    // Note: data is dropped if terminal is not connected
    usbd_edpt_release(rhport, p_itf->ep_in);
    return 0;
  }
}

uint32_t tud_vendor_n_write_available (uint8_t itf)
{
  return tu_fifo_remaining(&_vendord_itf[itf].tx_ff);
}

//--------------------------------------------------------------------+
// USBD Driver API
//--------------------------------------------------------------------+
void vendord_init(void)
{
  tu_memclr(_vendord_itf, sizeof(_vendord_itf));

  for(uint8_t i=0; i<CFG_TUD_VENDOR; i++)
  {
    vendord_interface_t* p_itf = &_vendord_itf[i];

    // config fifo
    tu_fifo_config(&p_itf->rx_ff, p_itf->rx_ff_buf, CFG_TUD_VENDOR_RX_BUFSIZE, 1, false);
    tu_fifo_config(&p_itf->tx_ff, p_itf->tx_ff_buf, CFG_TUD_VENDOR_TX_BUFSIZE, 1, false);

#if CFG_FIFO_MUTEX
    tu_fifo_config_mutex(&p_itf->rx_ff, NULL, osal_mutex_create(&p_itf->rx_ff_mutex));
    tu_fifo_config_mutex(&p_itf->tx_ff, osal_mutex_create(&p_itf->tx_ff_mutex), NULL);
#endif
  }
}

void vendord_reset(uint8_t rhport)
{
  (void) rhport;

  for(uint8_t i=0; i<CFG_TUD_VENDOR; i++)
  {
    vendord_interface_t* p_itf = &_vendord_itf[i];

    tu_memclr(p_itf, ITF_MEM_RESET_SIZE);
    tu_fifo_clear(&p_itf->rx_ff);
    tu_fifo_clear(&p_itf->tx_ff);
  }
}

uint16_t vendord_open(uint8_t rhport, tusb_desc_interface_t const * desc_itf, uint16_t max_len)
{
  TU_VERIFY(TUSB_CLASS_VENDOR_SPECIFIC == desc_itf->bInterfaceClass, 0);

  uint8_t const * p_desc = tu_desc_next(desc_itf);
  uint8_t const * desc_end = p_desc + max_len;

  // Find available interface
  vendord_interface_t* p_vendor = NULL;
  for(uint8_t i=0; i<CFG_TUD_VENDOR; i++)
  {
    if ( _vendord_itf[i].ep_in == 0 && _vendord_itf[i].ep_out == 0 )
    {
      p_vendor = &_vendord_itf[i];
      break;
    }
  }
  TU_VERIFY(p_vendor, 0);

  p_vendor->itf_num = desc_itf->bInterfaceNumber;
  if (desc_itf->bNumEndpoints)
  {
    // skip non-endpoint descriptors
    while ( (TUSB_DESC_ENDPOINT != tu_desc_type(p_desc)) && (p_desc < desc_end) )
    {
      p_desc = tu_desc_next(p_desc);
    }

    // Open endpoint pair with usbd helper
    TU_ASSERT(usbd_open_edpt_pair(rhport, p_desc, desc_itf->bNumEndpoints, TUSB_XFER_BULK, &p_vendor->ep_out, &p_vendor->ep_in), 0);

    p_desc += desc_itf->bNumEndpoints*sizeof(tusb_desc_endpoint_t);

    // Prepare for incoming data
    if ( p_vendor->ep_out )
    {
      _prep_out_transaction(p_vendor);
    }

    if ( p_vendor->ep_in ) tud_vendor_n_write_flush((uint8_t)(p_vendor - _vendord_itf));
  }

  return (uint16_t) ((uintptr_t) p_desc - (uintptr_t) desc_itf);
}

bool vendord_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
  (void) rhport;
  (void) result;

  uint8_t itf = 0;
  vendord_interface_t* p_itf = _vendord_itf;

  for ( ; ; itf++, p_itf++)
  {
    if (itf >= TU_ARRAY_SIZE(_vendord_itf)) return false;

    if ( ( ep_addr == p_itf->ep_out ) || ( ep_addr == p_itf->ep_in ) ) break;
  }

  if ( ep_addr == p_itf->ep_out )
  {
    // Receive new data
    tu_fifo_write_n(&p_itf->rx_ff, p_itf->epout_buf, (uint16_t) xferred_bytes);

    // Invoked callback if any
    if (tud_vendor_rx_cb) tud_vendor_rx_cb(itf);

    _prep_out_transaction(p_itf);
  }
  else if ( ep_addr == p_itf->ep_in )
  {
    if (tud_vendor_tx_cb) tud_vendor_tx_cb(itf, (uint16_t) xferred_bytes);
    // Send complete, try to send more if possible
    tud_vendor_n_write_flush(itf);
  }

  return true;
}

#endif
