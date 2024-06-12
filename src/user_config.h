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
 * 
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
 * ENABLE_ACCELERATION: [0-1], disables or enables mouse acceleration.
 * 
 * */

/* Output A values */
#define MOUSE_SPEED_A_FACTOR_X 16
#define MOUSE_SPEED_A_FACTOR_Y 16

/* Output B values  */
#define MOUSE_SPEED_B_FACTOR_X 16
#define MOUSE_SPEED_B_FACTOR_Y 16

#define JUMP_THRESHOLD 0

/* Mouse acceleration */
#define ENABLE_ACCELERATION 1


/**================================================== *
 * ===========  Mouse General Settings  ============= *
 * ================================================== *
 *
 * MOUSE_PARKING_POSITION: [0, 1, 2 ] 0 means park mouse on TOP
 *                                    1 means park mouse on BOTTOM
 *                                    2 means park mouse on PREVIOUS position
 * 
 * */

#define MOUSE_PARKING_POSITION 0


/**================================================== *
 * ==============  Screensaver Config  ============== *
 * ================================================== *
 *
 * While this feature is called 'screensaver', it's not actually a
 * screensaver :) Really it's a way to ensure that some sort of mouse
 * activity will be sent to one (or both) outputs when the user has
 * not interacted with that output. This can be used to stop a
 * screensaver or screenlock from activating on the attached computer,
 * or to just watch the mouse pointer bouncing around!
 *
 * When the mode is active on an output, the pointer will jump around
 * the screen like a bouncing-ball in a Pong game (however no click
 * events will be generated, of course).
 *
 * This mode is activated by 'idle time' on a per-output basis; if the
 * mode is enabled for output B, and output B doesn't have any
 * activity for (at least) the specified idle time, then the mode will
 * be activated and will continue until the inactivity time reaches
 * the maximum (if one has been specified). This allows you to stop a
 * screensaver/screenlock from activating while you are still at your
 * desk (but just interacting with the other computer attached to
 * Deskhop), but let it activate if you leave your desk for an
 * extended period of time.
 *
 * Additionally, this mode can be automatically disabled if the output
 * is the currently-active output.
 *
 * If you only set the ENABLED options below, and leave the rest of
 * the defaults in place, then the screensaver mode will activate
 * after 4 minutes (240 seconds) of inactivity, will continue forever,
 * but will only activate on an output that is not currently
 * active.
 *
 **/

/**================================================== *
 *
 * SCREENSAVER_{A|B}_ENABLED: [0 or 1] 0 means screensaver is
 * disabled, 1 means it is enabled.
 *
 **/

#define SCREENSAVER_A_ENABLED 0
#define SCREENSAVER_B_ENABLED 0

/**================================================== *
 *
 * SCREENSAVER_{A|B}_IDLE_TIME_SEC: Number of seconds that an output
 * must be inactive before the screensaver mode will be activated.
 *
 **/

#define SCREENSAVER_A_IDLE_TIME_SEC 240
#define SCREENSAVER_B_IDLE_TIME_SEC 240

/**================================================== *
 *
 * SCREENSAVER_{A|B}_MAX_TIME_SEC: Number of seconds that the screensaver
 * will run on an output before being deactivated. 0 for indefinitely.
 *
 **/

#define SCREENSAVER_A_MAX_TIME_SEC 0
#define SCREENSAVER_B_MAX_TIME_SEC 0

/**================================================== *
 *
 * SCREENSAVER_{A|B}_ONLY_IF_INACTIVE: [0 or 1] 1 means the
 * screensaver will activate only if the output is inactive.
 *
 **/

#define SCREENSAVER_A_ONLY_IF_INACTIVE 0
#define SCREENSAVER_B_ONLY_IF_INACTIVE 0

/**================================================== *
 * ================  Output OS Config =============== *
 * ==================================================
 *
 * Defines OS an output connects to. You will need to worry about this only if you have
 * multiple desktops and one of your outputs is MacOS or Windows.
 *
 * Available options: LINUX, MACOS, WINDOWS, OTHER (check main.h for details)
 *
 * OUTPUT_A_OS: OS for output A
 * OUTPUT_B_OS: OS for output B
 *
 * */

#define OUTPUT_A_OS LINUX
#define OUTPUT_B_OS LINUX

/**================================================== *
 * =================  Enforce Ports ================= *
 * ==================================================
 *
 * If enabled, fixes some device incompatibilities by
 * enforcing keyboard has to be in port A and mouse in port B.
 *
 * ENFORCE_PORTS: [0, 1] - 1 means keyboard has to plug in A and mouse in B
 *                         0 means no such layout is enforced
 *
 * */

#define ENFORCE_PORTS 0