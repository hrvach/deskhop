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

#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hid_parser.h"
#include "pio_usb.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "user_config.h"
#include "protocol.h"
#include <hardware/structs/ioqspi.h>
#include <hardware/structs/sio.h>
#include <hardware/dma.h>
#include <hardware/flash.h>
#include <hardware/sync.h>
#include <hardware/watchdog.h>
#include <pico/bootrom.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <pico/util/queue.h>

/*********  Misc definitions for better readability **********/
#define OUTPUT_A 0
#define OUTPUT_B 1

#define ENABLE  1
#define DISABLE 0

#define ABSOLUTE 0
#define RELATIVE 1
#define TOUCH 2

#define MOUSE_BOOT_REPORT_LEN 4
#define MOUSE_ZOOM_SCALING_FACTOR 2
#define NUM_SCREENS               2   // Will be more in the future
#define CONFIG_MODE_TIMEOUT 300000000 // 5 minutes into the future

#define ARRAY_SIZE(arr)                (sizeof(arr) / sizeof((arr)[0]))
#define CURRENT_BOARD_IS_ACTIVE_OUTPUT (global_state.active_output == global_state.board_role)

#define _TOP()  0
#define _MS(x) (x * 1000)
#define _SEC(x) (x * 1000000)
#define _HZ(x) ((uint64_t)((1000000) / (x)))

/*********  Pinout definitions  **********/
#define PIO_USB_DP_PIN 14 // D+ is pin 14, D- is pin 15
#define GPIO_LED_PIN   25 // LED is connected to pin 25 on a PICO

#define BOARD_A_RX 13
#define BOARD_A_TX 12
#define BOARD_B_RX 17
#define BOARD_B_TX 16

#define SERIAL_TX_PIN (global_state.board_role == OUTPUT_A ? BOARD_A_TX : BOARD_B_TX)
#define SERIAL_RX_PIN (global_state.board_role == OUTPUT_A ? BOARD_A_RX : BOARD_B_RX)

#define BOARD_ROLE (global_state.board_role)
#define OTHER_ROLE (BOARD_ROLE == OUTPUT_A ? OUTPUT_B : OUTPUT_A)

/*********  Serial port definitions  **********/
#define SERIAL_UART     uart0
#define SERIAL_BAUDRATE 3686400

#define SERIAL_DATA_BITS 8
#define SERIAL_STOP_BITS 1
#define SERIAL_PARITY    UART_PARITY_NONE

/*********  DMA  definitions  **********/
#define DMA_RX_BUFFER_SIZE 1024
#define DMA_TX_BUFFER_SIZE 32

extern uint8_t uart_rxbuf[DMA_RX_BUFFER_SIZE] __attribute__((aligned(DMA_RX_BUFFER_SIZE)));
extern uint8_t uart_txbuf[DMA_TX_BUFFER_SIZE] __attribute__((aligned(DMA_TX_BUFFER_SIZE)));

/*********  Watchdog definitions  **********/
#define WATCHDOG_TIMEOUT        500                     // In milliseconds => needs to be reset at least every 200ms
#define WATCHDOG_PAUSE_ON_DEBUG 1                       // When using a debugger, disable watchdog
#define CORE1_HANG_TIMEOUT_US   WATCHDOG_TIMEOUT * 1000 // Convert to microseconds

#define MAGIC_WORD_1 0xdeadf00f // When these are set, we'll boot to configuration mode
#define MAGIC_WORD_2 0x00c0ffee

/*********  Protocol definitions  *********
 *
 * - every packet starts with 0xAA 0x55 for easy re-sync
 * - then a 1 byte packet type is transmitted
 * - 8 bytes of packet data follows, fixed length for simplicity
 * - 1 checksum byte ends the packet
 *      - checksum includes **only** the packet data
 *      - checksum is simply calculated by XORing all bytes together
 */

enum packet_type_e {
    KEYBOARD_REPORT_MSG  = 1,
    MOUSE_REPORT_MSG     = 2,
    OUTPUT_SELECT_MSG    = 3,
    FIRMWARE_UPGRADE_MSG = 4,
    MOUSE_ZOOM_MSG       = 5,
    KBD_SET_REPORT_MSG   = 6,
    SWITCH_LOCK_MSG      = 7,
    SYNC_BORDERS_MSG     = 8,
    FLASH_LED_MSG        = 9,
    WIPE_CONFIG_MSG      = 10,
    HEARTBEAT_MSG        = 12,
    GAMING_MODE_MSG      = 13,
    CONSUMER_CONTROL_MSG = 14,
    SYSTEM_CONTROL_MSG   = 15,
    SAVE_CONFIG_MSG      = 18,
    REBOOT_MSG           = 19,
    GET_VAL_MSG          = 20,
    SET_VAL_MSG          = 21,
    GET_ALL_VALS_MSG     = 22,
    PROXY_PACKET_MSG     = 23,
    REQUEST_BYTE_MSG     = 24,
    RESPONSE_BYTE_MSG    = 25,
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
    uint8_t type;     // Enum field describing the type of packet
    union {
        uint8_t data[8];      // Data goes here (type + payload + checksum)
        uint16_t data16[4];   // We can treat it as 4 16-byte chunks
        uint32_t data32[2];   // We can treat it as 2 32-byte chunks
    };
    uint8_t checksum; // Checksum, a simple XOR-based one
} __attribute__((packed)) uart_packet_t;

/*********  Packet parameters  **********/

#define START1       0xAA
#define START2       0x55
#define START_LENGTH 2

#define TYPE_LENGTH        1
#define PACKET_DATA_LENGTH 8 // For simplicity, all packet types are the same length
#define CHECKSUM_LENGTH    1

#define PACKET_LENGTH     (TYPE_LENGTH + PACKET_DATA_LENGTH + CHECKSUM_LENGTH)
#define RAW_PACKET_LENGTH (START_LENGTH + PACKET_LENGTH)

#define UART_QUEUE_LENGTH  256
#define HID_QUEUE_LENGTH   128
#define KBD_QUEUE_LENGTH   128
#define MOUSE_QUEUE_LENGTH 512

#define KEYARRAY_BIT_OFFSET     16
#define KEYS_IN_USB_REPORT      6
#define KBD_REPORT_LENGTH       8
#define MOUSE_REPORT_LENGTH     8
#define CONSUMER_CONTROL_LENGTH 4
#define SYSTEM_CONTROL_LENGTH   1
#define MODIFIER_BIT_LENGTH     8

/*********  Screen  **********/
#define MIN_SCREEN_COORD 0
#define MAX_SCREEN_COORD 32767

/*********  Configuration storage definitions  **********/

#define CURRENT_CONFIG_VERSION 8

enum os_type_e {
    LINUX   = 1,
    MACOS   = 2,
    WINDOWS = 3,
    ANDROID = 4,
    OTHER   = 255,
};

enum screen_pos_e {
    LEFT   = 1,
    RIGHT  = 2,
    MIDDLE = 3,
};

enum screensaver_mode_e {
    DISABLED = 0,
    PONG     = 1,
    JITTER   = 2,
};

#define ITF_NUM_HID        0
#define ITF_NUM_HID_REL_M  1
#define ITF_NUM_HID_VENDOR 1
#define ITF_NUM_MSC        2

typedef struct {
    int top;    // When jumping from a smaller to a bigger screen, go to THIS top height
    int bottom; // When jumping from a smaller to a bigger screen, go to THIS bottom
                // height
} border_size_t;

/* Define screensaver parameters */
typedef struct {
    uint8_t mode;
    uint8_t only_if_inactive;
    uint64_t idle_time_us;
    uint64_t max_time_us;
} screensaver_t;

/* Define output parameters */
typedef struct {
    uint32_t number;           // Number of this output (e.g. OUTPUT_A = 0 etc)
    uint32_t screen_count;     // How many monitors per output (e.g. Output A is Windows with 3 monitors)
    uint32_t screen_index;     // Current active screen
    int32_t speed_x;           // Mouse speed per output, in direction X
    int32_t speed_y;           // Mouse speed per output, in direction Y
    border_size_t border;      // Screen border size/offset to keep cursor at same height when switching
    uint8_t os;                // Operating system on this output
    uint8_t pos;               // Screen position on this output
    uint8_t mouse_park_pos;    // Where the mouse goes after switch
    screensaver_t screensaver; // Screensaver parameters for this output
} output_t;

/* Data structure defining how configuration is stored */
typedef struct {
    uint32_t magic_header;
    uint32_t version;

    uint8_t force_mouse_boot_mode;
    uint8_t force_kbd_boot_protocol;

    uint8_t kbd_led_as_indicator;
    uint8_t hotkey_toggle;
    uint8_t enable_acceleration;

    uint8_t enforce_ports;
    uint16_t jump_treshold;

    output_t output[NUM_SCREENS];
    uint32_t _reserved;

    // Keep checksum at the end of the struct
    uint32_t checksum;
} config_t;

extern const config_t default_config;

/*********  Flash data section  **********/
typedef struct {
    uint8_t cmd;          // Byte 0 = command
    uint16_t page_number; // Bytes 1-2 = page number
    union {
        uint8_t offset;   // Byte 3 = offset
        uint8_t checksum; // In write packets, it's checksum
    };
    uint8_t data[4]; // Bytes 4-7 = data
} fw_packet_t;

extern const config_t ADDR_CONFIG[];
extern const uint8_t ADDR_FW_METADATA[];
extern const uint8_t ADDR_FW_RUNNING[];
extern const uint8_t ADDR_FW_STAGING[];
extern const uint8_t ADDR_DISK_IMAGE[];

/* Ring buffer wraps around after reaching 4095 */
#define NEXT_RING_IDX(x) ((x + 1) & 0x3FF)

typedef struct {
    uint16_t magic;
    uint16_t version;
    uint32_t checksum;
} firmware_metadata_t;

extern firmware_metadata_t _firmware_metadata;

#define FIRMWARE_METADATA_MAGIC   0xf00d
#define RUNNING_FIRMWARE_SLOT     0
#define STAGING_FIRMWARE_SLOT     1

#define STAGING_PAGES_CNT         1024
#define STAGING_IMAGE_SIZE        STAGING_PAGES_CNT * FLASH_PAGE_SIZE

extern const uint32_t crc32_lookup_table[];


typedef struct {
    uint32_t magicStart0;
    uint32_t magicStart1;
    uint32_t flags;
    uint32_t targetAddr;
    uint32_t payloadSize;
    uint32_t blockNo;
    uint32_t numBlocks;
    uint32_t fileSize;
    uint8_t data[476];
    uint32_t magicEnd;
} uf2_t;

#define UF2_MAGIC_START0 0x0A324655
#define UF2_MAGIC_START1 0x9E5D5157
#define UF2_MAGIC_END    0x0AB16F30

// -------------------------------------------------------+

typedef void (*action_handler_t)();

typedef struct { // Maps message type -> message handler function
    enum packet_type_e type;
    action_handler_t handler;
} uart_handler_t;

typedef struct {
    uint8_t modifier;                 // Which modifier is pressed
    uint8_t keys[KEYS_IN_USB_REPORT]; // Which keys need to be pressed
    uint8_t key_count;                // How many keys are pressed
    action_handler_t action_handler;  // What to execute when the key combination is detected
    bool pass_to_os;                  // True if we are to pass the key to the OS too
    bool acknowledge;                 // True if we are to notify the user about registering keypress
} hotkey_combo_t;

typedef struct TU_ATTR_PACKED {
    uint8_t buttons;
    int16_t x;
    int16_t y;
    int8_t wheel;
    int8_t pan;
    uint8_t mode;
} mouse_report_t;

/* Used to work around OS issues with absolute coordinates on
   multiple desktops (Windows/MacOS) */
typedef struct {
    uint8_t tip_pressure;
    uint8_t buttons; // Digitizer buttons
    uint16_t x;      // X coordinate (0-32767)
    uint16_t y;      // Y coordinate (0-32767)
} touch_report_t;

/* This stores various packets other than kbd/mouse to go out
   (configuration, consumer control, system...) */
typedef struct {
    uint8_t instance;
    uint8_t report_id;
    uint8_t type;
    uint8_t len;
    uint8_t data[RAW_PACKET_LENGTH];
} hid_generic_pkt_t;

typedef enum { IDLE, READING_PACKET, PROCESSING_PACKET } receiver_state_t;

typedef struct {
    uint32_t address;         // Address we're sending to the other box
    uint32_t checksum;
    uint16_t version;
    bool byte_done;           // Has the byte been successfully transferred
    bool upgrade_in_progress; // True if firmware transfer from the other box is in progress
} fw_upgrade_state_t;

typedef struct {
    uint8_t kbd_dev_addr; // Address of the keyboard device
    uint8_t kbd_instance; // Keyboard instance (d'uh - isn't this a useless comment)

    uint8_t keyboard_leds[NUM_SCREENS];  // State of keyboard LEDs (index 0 = A, index 1 = B)
    uint64_t last_activity[NUM_SCREENS]; // Timestamp of the last input activity (-||-)
    uint64_t core1_last_loop_pass;       // Timestamp of last core1 loop execution
    uint8_t active_output;               // Currently selected output (0 = A, 1 = B)
    uint8_t board_role;                  // Which board are we running on? (0 = A, 1 = B, etc.)

    int16_t pointer_x; // Store and update the location of our mouse pointer
    int16_t pointer_y;
    int16_t mouse_buttons; // Store and update the state of mouse buttons

    config_t config;       // Device configuration, loaded from flash or defaults used
    queue_t hid_queue_out; // Queue that stores outgoing hid messages
    queue_t kbd_queue;     // Queue that stores keyboard reports
    queue_t mouse_queue;   // Queue that stores mouse reports
    queue_t uart_tx_queue; // Queue that stores outgoing packets

    hid_interface_t iface[MAX_DEVICES][MAX_INTERFACES]; // Store info about HID interfaces
    uart_packet_t in_packet;

    /* DMA */
    uint32_t dma_ptr;             // Stores info about DMA ring buffer last checked position
    uint32_t dma_rx_channel;      // DMA RX channel we're using to receive
    uint32_t dma_control_channel; // DMA channel that controls the RX transfer channel
    uint32_t dma_tx_channel;      // DMA TX channel we're using to send

    /* Firmware */
    fw_upgrade_state_t fw;           // State of the firmware upgrader
    firmware_metadata_t _running_fw; // RAM copy of running fw metadata
    bool reboot_requested;           // If set, stop updating watchdog
    uint64_t config_mode_timer;      // Counts how long are we to remain in config mode

    uint8_t page_buffer[FLASH_PAGE_SIZE]; // For firmware-over-serial upgrades

    /* Connection status flags */
    bool tud_connected;      // True when TinyUSB device successfully connects
    bool keyboard_connected; // True when our keyboard is connected locally

    /* Feature flags */
    bool mouse_zoom;         // True when "mouse zoom" is enabled
    bool switch_lock;        // True when device is prevented from switching
    bool onboard_led_state;  // True when LED is ON
    bool relative_mouse;     // True when relative mouse mode is used
    bool gaming_mode;        // True when gaming mode is on (relative passthru + lock)
    bool config_mode_active; // True when config mode is active
    bool digitizer_active;   // True when digitizer Win/Mac workaround is active

    /* Onboard LED blinky (provide feedback when e.g. mouse connected) */
    int32_t blinks_left;     // How many blink transitions are left
    int32_t last_led_change; // Timestamp of the last time led state transitioned
} device_t;

typedef struct {
    void (*exec)(device_t *state);
    uint64_t frequency;
    uint64_t next_run;
    bool *enabled;
} task_t;

/*********  Setup  **********/
void initial_setup(device_t *);
void serial_init(void);
void core1_main(void);

/*********  Keyboard  **********/
bool check_specific_hotkey(hotkey_combo_t, const hid_keyboard_report_t *);
void process_keyboard_report(uint8_t *, int, uint8_t, hid_interface_t *);
void process_consumer_report(uint8_t *, int, uint8_t, hid_interface_t *);
void process_system_report(uint8_t *, int, uint8_t, hid_interface_t *);
void release_all_keys(device_t *);
void queue_kbd_report(hid_keyboard_report_t *, device_t *);
void queue_cc_packet(uint8_t *, device_t *);
void queue_system_packet(uint8_t *, device_t *);
void send_key(hid_keyboard_report_t *, device_t *);
void send_consumer_control(uint8_t *, device_t *);
bool key_in_report(uint8_t, const hid_keyboard_report_t *);
int32_t extract_bit_variable(uint32_t, uint32_t, uint8_t *, int, uint8_t *);
int32_t extract_kbd_data(uint8_t *, int, uint8_t, hid_interface_t *, hid_keyboard_report_t *);

/*********  Mouse  **********/
bool tud_mouse_report(uint8_t mode, uint8_t buttons, int16_t x, int16_t y, int8_t wheel, int8_t pan);

void process_mouse_report(uint8_t *, int, uint8_t, hid_interface_t *);
void parse_report_descriptor(hid_interface_t *, uint8_t const *, int);
void extract_data(hid_interface_t *, report_val_t *);
int32_t get_report_value(uint8_t *report, report_val_t *val);
void queue_mouse_report(mouse_report_t *, device_t *);
void output_mouse_report(mouse_report_t *, device_t *);

/*********  UART  **********/
void process_packet(uart_packet_t *, device_t *);
void queue_packet(const uint8_t *, enum packet_type_e, int);
void write_raw_packet(uint8_t *, uart_packet_t *);
void send_value(const uint8_t, enum packet_type_e);
bool get_packet_from_buffer(device_t *);

/*********  LEDs  **********/
void restore_leds(device_t *);
void blink_led(device_t *);
uint8_t toggle_led(void);

/*********  Checksum  **********/
uint8_t calc_checksum(const uint8_t *, int);
bool verify_checksum(const uart_packet_t *);
uint32_t crc32(const uint8_t *, size_t);
uint32_t crc32_iter(uint32_t, const uint8_t);

/*********  Firmware  **********/
void write_flash_page(uint32_t, uint8_t *);
uint32_t calculate_firmware_crc32(void);
bool is_bootsel_pressed(void);
void request_byte(device_t *, uint32_t);
uint32_t get_ptr_delta(uint32_t, device_t *);
bool is_start_of_packet(device_t *);
void fetch_packet(device_t *);
void reboot(void);

/*********  Tasks  **********/
void process_uart_tx_task(device_t *);
void process_mouse_queue_task(device_t *);
void process_hid_queue_task(device_t *);
void process_kbd_queue_task(device_t *);
void usb_device_task(device_t *);
void kick_watchdog_task(device_t *);
void usb_host_task(device_t *);
void packet_receiver_task(device_t *);
void screensaver_task(device_t *);
void firmware_upgrade_task(device_t *);
void heartbeat_output_task(device_t *);
void led_blinking_task(device_t *);

void task_scheduler(device_t *, task_t *);

/*********  Configuration  **********/
void load_config(device_t *);
void save_config(device_t *);
void wipe_config(void);
void reset_config_timer(device_t *);

extern const field_map_t api_field_map[];
const field_map_t* get_field_map_entry(uint32_t);
const field_map_t* get_field_map_index(uint32_t);
size_t get_field_map_length(void);
bool validate_packet(uart_packet_t *);
void queue_cfg_packet(uart_packet_t *, device_t *);

/*********  Handlers  **********/
void output_toggle_hotkey_handler(device_t *, hid_keyboard_report_t *);
void screen_border_hotkey_handler(device_t *, hid_keyboard_report_t *);
void fw_upgrade_hotkey_handler_A(device_t *, hid_keyboard_report_t *);
void fw_upgrade_hotkey_handler_B(device_t *, hid_keyboard_report_t *);
void mouse_zoom_hotkey_handler(device_t *, hid_keyboard_report_t *);
void all_keys_released_handler(device_t *);
void switchlock_hotkey_handler(device_t *, hid_keyboard_report_t *);
void toggle_gaming_mode_handler(device_t *, hid_keyboard_report_t *);
void screenlock_hotkey_handler(device_t *, hid_keyboard_report_t *);
void output_config_hotkey_handler(device_t *, hid_keyboard_report_t *);
void wipe_config_hotkey_handler(device_t *, hid_keyboard_report_t *);
void config_enable_hotkey_handler(device_t *, hid_keyboard_report_t *);

void handle_keyboard_uart_msg(uart_packet_t *, device_t *);
void handle_mouse_abs_uart_msg(uart_packet_t *, device_t *);
void handle_output_select_msg(uart_packet_t *, device_t *);
void handle_mouse_zoom_msg(uart_packet_t *, device_t *);
void handle_set_report_msg(uart_packet_t *, device_t *);
void handle_switch_lock_msg(uart_packet_t *, device_t *);
void handle_sync_borders_msg(uart_packet_t *, device_t *);
void handle_flash_led_msg(uart_packet_t *, device_t *);
void handle_fw_upgrade_msg(uart_packet_t *, device_t *);
void handle_wipe_config_msg(uart_packet_t *, device_t *);
void handle_consumer_control_msg(uart_packet_t *, device_t *);
void handle_read_config_msg(uart_packet_t *, device_t *);
void handle_save_config_msg(uart_packet_t *, device_t *);
void handle_reboot_msg(uart_packet_t *, device_t *);
void handle_write_fw_msg(uart_packet_t *, device_t *);
void handle_request_byte_msg(uart_packet_t *, device_t *);
void handle_response_byte_msg(uart_packet_t *, device_t *);
void handle_heartbeat_msg(uart_packet_t *, device_t *);
void handle_proxy_msg(uart_packet_t *, device_t *);
void handle_api_msgs(uart_packet_t *, device_t *);
void handle_api_read_all_msg(uart_packet_t *, device_t *);
void handle_toggle_gaming_msg(uart_packet_t *, device_t *);

void switch_output(device_t *, uint8_t);

/*********  Global variables (don't judge)  **********/
extern device_t global_state;
