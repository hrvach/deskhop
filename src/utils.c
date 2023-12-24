#include "main.h"

/**================================================== *
 * ==============  Checksum Functions  ============== *
 * ================================================== */

uint8_t calc_checksum(const uint8_t* data, int length) {    
    uint8_t checksum = 0;

    for (int i = 0; i < length; i++) {
        checksum ^= data[i];
    }

    return checksum;
}

bool verify_checksum(const uart_packet_t* packet) {
    uint8_t checksum = calc_checksum(packet->data, PACKET_DATA_LENGTH);
    return checksum == packet->checksum;
}

/**================================================== *
 * ==============  Watchdog Functions  ============== *
 * ================================================== */
       
void kick_watchdog(void) {
    // Read the timer AFTER duplicating the core1 timestamp, 
    // so it doesn't get updated in the meantime.

    uint64_t core1_last_loop_pass = global_state.core1_last_loop_pass;
    uint64_t current_time = time_us_64();

    // If core1 stops updating the timestamp, we'll stop kicking the watchog and reboot
    if (current_time - core1_last_loop_pass < CORE1_HANG_TIMEOUT_US)        
        watchdog_update();
}
    
