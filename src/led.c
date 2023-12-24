#include "main.h"

/**==================================================== *
 * ========== Update pico and keyboard LEDs  ========== *
 * ==================================================== */

void update_leds(device_state_t* state) {
    gpio_put(GPIO_LED_PIN, state->active_output == BOARD_ROLE);

    if (BOARD_ROLE == KEYBOARD_PICO_A)
        pio_usb_kbd_set_leds(state->usb_device, 0, state->keyboard_leds[state->active_output]);
}
