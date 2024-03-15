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
 * If you do not want to use a key for switching outputs, you may be tempted
 * to select HID_KEY_NONE here; don't do that! That code appears in many HID
 * messages and the result will be a non-functional keyboard. Instead, choose
 * a key that is unlikely to ever appear on a keyboard that you will use.
 * HID_KEY_F24 is probably a good choice as keyboards with 24 function keys
 * are rare.
 * */

#define HOTKEY_TOGGLE HID_KEY_CAPS_LOCK

/**================================================== *
 * ==============  Mouse Speed Factor  ============== *
 * ==================================================
 *
 * This affects how fast the mouse moves.
 *
 * MOUSE_SPEED_A_FACTOR_X: [1-128], mouse moves at this speed in X direction
 * MOUSE_SPEED_A_FACTOR_Y: [1-128], mouse moves at this speed in Y direction
 *
 * JUMP_THRESHOLD: [0-32768], sets the "force" you need to use to drag the
 * mouse to another screen, 0 meaning no force needed at all, and ~500 some force
 * needed, ~1000 no accidental jumps, you need to really mean it.
 *
 * This is now configurable per-screen.
 *
 * */

/* Output A values */
#define MOUSE_SPEED_A_FACTOR_X 16
#define MOUSE_SPEED_A_FACTOR_Y 16

/* Output B values */
#define MOUSE_SPEED_B_FACTOR_X 16
#define MOUSE_SPEED_B_FACTOR_Y 16

#define JUMP_THRESHOLD 0


/**================================================== *
 * ==============  Screensaver Config  ============== *
 * ==================================================
 *
 * Defines how long does an output need to be idle for screensaver to kick in.
 * With this function, after being left idle for a certain amount of time (defined below),
 * mouse cursor starts moving around like a bouncy-ball in pong. No clicking, of course.
 * Move mouse on that active output to stop.
 *
 * SCREENSAVER_ENABLED: [0 or 1] 0 means screensaver is disabled, 1 means it is enabled.
 * SCREENSAVER_TIME_SEC: time in seconds
 *
 * */

#define SCREENSAVER_ENABLED  0
#define SCREENSAVER_TIME_SEC 240
