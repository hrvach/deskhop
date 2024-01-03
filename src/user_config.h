/**===================================================== *
 * ==========  Keyboard LED Output Indicator  ========== *
 * ===================================================== *
 *
 * If you are willing to give up on using the keyboard LEDs for their original purpose,
 * you can use them as a convenient way to indicate which output is selected.
 *
 * KBD_LED_AS_INDICATOR set to 0 will use the keyboard LEDs as normal.
 * KBD_LED_AS_INDICATOR set to 1 will use the Caps Lock LED as indicator.
 *
 * */

#define KBD_LED_AS_INDICATOR 0

/**===================================================== *
 * ===========  Hotkey for output switching  =========== *
 * ===================================================== *
 *
 * Everyone is different, I prefer to use caps lock because I HATE SHOUTING :)
 * You might prefer something else. Pick something from the list found at:
 *
 * https://github.com/hathach/tinyusb/blob/master/src/class/hid/hid.h
 *
 * defined as HID_KEY_<something>
 *
 * */

#define HOTKEY_TOGGLE HID_KEY_CAPS_LOCK

/**================================================== *
 * ==============  Mouse Speed Factor  ============== *
 * ==================================================
 *
 * This affects how fast the mouse moves.
 *
 * MOUSE_SPEED_FACTOR_X: [1-128], mouse moves at this speed in X direction
 * MOUSE_SPEED_FACTOR_Y: [1-128], mouse moves at this speed in Y direction
 * 
 * MOUSE_JUMP_THRESHOLD: [0-32768], sets the "force" you need to use to drag the 
 * mouse to another screen, 0 meaning no force needed at all, and ~500 some force
 * needed, ~1000 no accidental jumps, you need to really mean it.
 * 
 * TODO: make this configurable per-screen.
 *
 * */

#define MOUSE_SPEED_FACTOR_X 1
#define MOUSE_SPEED_FACTOR_Y 1

#define MOUSE_JUMP_THRESHOLD 0
