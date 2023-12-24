#pragma once

#include "pico/stdlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/watchdog.h"
#include "pico/bootrom.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "pio_usb.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "user_config.h"

/*********  Misc definitions  **********/
#define KEYBOARD_PICO_A 0
#define MOUSE_PICO_B 1

#define ACTIVE_OUTPUT_A 0
#define ACTIVE_OUTPUT_B 1

#define ENABLE 1
#define DISABLE 0

/*********  Pinout definitions  **********/
#define PIO_USB_DP_PIN 14  // D+ is pin 14, D- is pin 15
#define GPIO_LED_PIN 25    // LED is connected to pin 25 on a PICO

#if BOARD_ROLE == MOUSE_PICO_B
#define SERIAL_TX_PIN 16
#define SERIAL_RX_PIN 17
#elif BOARD_ROLE == KEYBOARD_PICO_A
#define SERIAL_TX_PIN 12
#define SERIAL_RX_PIN 13
#endif

/*********  Serial port definitions  **********/
#define SERIAL_UART uart0
#define SERIAL_BAUDRATE 115200
#define SERIAL_DATA_BITS 8
#define SERIAL_STOP_BITS 1
#define SERIAL_PARITY UART_PARITY_NONE

/*********  Watchdog definitions  **********/
#define WATCHDOG_TIMEOUT 500          // In milliseconds => needs to be reset every 500 ms
#define WATCHDOG_PAUSE_ON_DEBUG 1     // When using a debugger, disable watchdog
#define CORE1_HANG_TIMEOUT_US 500000  // In microseconds, wait up to 0.5s to declare core1 dead

/*********  Protocol definitions  *********
 *
 * - every packet starts with 0xAA 0x55 for easy re-sync
 * - then a 1 byte packet type is transmitted
 * - 8 bytes of packet data follows, fixed length for simplicity
 * - 1 checksum byte ends the packet
 *      - checksum includes **only** the packet data
 *      - checksum is simply calculated by XORing all bytes together
 */

enum packet_type_e : uint8_t {
    KEYBOARD_REPORT_MSG = 1,
    MOUSE_REPORT_MSG = 2,
    OUTPUT_SELECT_MSG = 3,
    FIRMWARE_UPGRADE_MSG = 4,
    MOUSE_ZOOM_MSG = 5,
    KBD_SET_REPORT_MSG = 6,
};

/*
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| Start1 | Start2 | Type |             Packet data           | Checksum |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|    1   |    1   |  1   |                 8                 |     1    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

/* Data structure defining packets of information transferred */
typedef struct {
    uint8_t type;      // Enum field describing the type of packet
    uint8_t data[8];   // Data goes here (type + payload + checksum)
    uint8_t checksum;  // Checksum, a simple XOR-based one
} uart_packet_t;

/*********  Packet parameters  **********/

#define START1 0xAA
#define START2 0x55
#define START_LENGTH 2

#define TYPE_LENGTH 1
#define PACKET_DATA_LENGTH 8  // For simplicity, all packet types are the same length
#define CHECKSUM_LENGTH 1

#define PACKET_LENGTH (TYPE_LENGTH + PACKET_DATA_LENGTH + CHECKSUM_LENGTH)
#define RAW_PACKET_LENGTH (START_LENGTH + PACKET_LENGTH)

#define KEYS_IN_USB_REPORT 6
#define KBD_REPORT_LENGTH 8
#define MOUSE_REPORT_LENGTH 7

/*********  Screen  **********/
#define MAX_SCREEN_COORD 32767

// -------------------------------------------------------+

typedef void (*action_handler_t)();

typedef struct {
    uint8_t modifier;                 // Which modifier is pressed
    uint8_t keys[6];                  // Which keys need to be pressed
    uint8_t key_count;                // How many keys are pressed
    action_handler_t action_handler;  // What to execute when the key combination is detected
} hotkey_combo_t;

typedef struct TU_ATTR_PACKED {
    uint8_t buttons;
    int16_t x;
    int16_t y;
    int8_t wheel;
    int8_t pan;
} hid_abs_mouse_report_t;

typedef enum { IDLE, READING_PACKET, PROCESSING_PACKET } receiver_state_t;

typedef struct {
    usb_device_t* usb_device;         // USB device structure (keyboard or mouse)
    uint8_t keyboard_leds[2];         // State of keyboard LEDs (index 0 = A, index 1 = B)
    uint64_t last_activity[2];        // Timestamp of the last input activity (-||-)
    receiver_state_t receiver_state;  // Storing the state for the simple receiver state machine

    uint64_t core1_last_loop_pass;  // Timestamp of last core1 loop execution
    uint8_t active_output;          // Currently selected output (0 = A, 1 = B)

    int16_t mouse_x;  // Store and update the location of our mouse pointer
    int16_t mouse_y;

    bool tud_connected;  // True when TinyUSB device successfully connects
    bool mouse_zoom;     // True when "mouse zoom" is enabled

} device_state_t;

/*******  USB Host  *********/
void process_mouse_report(uint8_t*, int, device_state_t*);
void check_endpoints(device_state_t* state);

/*********  Setup  **********/
usb_device_t* initial_setup(void);
void serial_init(void);
void core1_main(void);

/*********  Keyboard  **********/
bool keypress_check(hotkey_combo_t, const hid_keyboard_report_t*);
void process_keyboard_report(uint8_t*, int, device_state_t*);

/*********  Mouse  **********/
bool tud_hid_abs_mouse_report(uint8_t report_id,
                              uint8_t buttons,
                              int16_t x,
                              int16_t y,
                              int8_t vertical,
                              int8_t horizontal);

/*********  UART  **********/
void receive_char(uart_packet_t*, device_state_t*);
void send_packet(const uint8_t*, enum packet_type_e, int);
void send_value(const uint8_t, enum packet_type_e);

/*********  LEDs  **********/
void update_leds(device_state_t*);

/*********  Checksum  **********/
uint8_t calc_checksum(const uint8_t*, int);
bool verify_checksum(const uart_packet_t*);

/*********  Watchdog  **********/
void kick_watchdog(void);

/*********  Handlers  **********/
void output_toggle_hotkey_handler(device_state_t*);
void fw_upgrade_hotkey_handler(device_state_t*);
void mouse_zoom_hotkey_handler(device_state_t*);
void all_keys_released_handler(device_state_t*);

void handle_keyboard_uart_msg(uart_packet_t*, device_state_t*);
void handle_mouse_abs_uart_msg(uart_packet_t*, device_state_t*);
void handle_output_select_msg(uart_packet_t*, device_state_t*);
void handle_fw_upgrade_msg(void);
void handle_mouse_zoom_msg(uart_packet_t*, device_state_t*);
void handle_set_report_msg(uart_packet_t*, device_state_t*);

void switch_output(uint8_t);

/*********  Global variables (don't judge)  **********/
extern device_state_t global_state;
