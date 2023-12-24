#include "main.h"

/**================================================== *
 * ==========  Query endpoints for reports  ========= *
 * ================================================== */

void check_endpoints(device_state_t* state) {
    uint8_t raw_report[64];

    // Iterate through all endpoints and check for data
    for (int ep_idx = 0; ep_idx < PIO_USB_DEV_EP_CNT; ep_idx++) {
        endpoint_t* ep = pio_usb_get_endpoint(state->usb_device, ep_idx);

        if (ep == NULL) {
            continue;
        }

        int len = pio_usb_get_in_data(ep, raw_report, sizeof(raw_report));

        if (len > 0) {
            if (BOARD_ROLE == KEYBOARD_PICO_A)
                process_keyboard_report(raw_report, len, state);
            else
                process_mouse_report(raw_report, len, state);
        }
    }
}

/**================================================== *
 * ===========  TinyUSB Device Callbacks  =========== *
 * ================================================== */

uint16_t tud_hid_get_report_cb(uint8_t instance,
                               uint8_t report_id,
                               hid_report_type_t report_type,
                               uint8_t* buffer,
                               uint16_t reqlen) {
    return 0;
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
                           uint8_t const* buffer,
                           uint16_t bufsize) {
    if (report_id == REPORT_ID_KEYBOARD && bufsize == 1 && report_type == HID_REPORT_TYPE_OUTPUT) {
        /**
         * If we are using caps lock LED to indicate the chosen output, we will
         * override whatever is sent through the SetReport message.
         */
        uint8_t leds = buffer[0];

        if (KBD_LED_AS_INDICATOR) {
            leds = leds & 0xFD;  // 1111 1101 (Clear Caps Lock bit)

            if (global_state.active_output)
                leds |= KEYBOARD_LED_CAPSLOCK;
        }
        
        global_state.keyboard_leds[global_state.active_output] = leds;        

        // If we are board B, we need to set this information to the other one since that one
        // has the keyboard connected to it (and LEDs you can turn on :-))
        if (BOARD_ROLE == MOUSE_PICO_B)
            send_value(leds, KBD_SET_REPORT_MSG);

        // If we are board A, update LEDs directly
        else        
            update_leds(&global_state);
    }
}

// Invoked when device is mounted
void tud_mount_cb(void)
{
  global_state.tud_connected = true;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  global_state.tud_connected = false;
}