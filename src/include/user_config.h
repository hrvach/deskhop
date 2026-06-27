/*
 * This file is part of DeskHop (https://github.com/hrvach/deskhop).
 * Copyright (c) 2025 Hrvoje Cavrak
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * See the file LICENSE for the full license text.
 *
 **===================================================== *
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
 * In addition, there is an optional modifier you can use for the hotkey
 * switching. Currently, it's set to LEFT CTRL, you can change it by
 * redefining HOTKEY_MODIFIER here from KEYBOARD_MODIFIER_LEFTCTRL to something
 * else (check file referenced below) or HID_KEY_NONE.
 *
 * If you do not want to use a key for switching outputs, you may be tempted
 * to select HID_KEY_NONE here as well; don't do that! That code appears in many
 * HID messages and the result will be a non-functional keyboard. Instead, choose
 * a key that is unlikely to ever appear on a keyboard that you will use.
 * HID_KEY_F24 is probably a good choice as keyboards with 24 function keys
 * are rare.
 *
 * */

#define HOTKEY_MODIFIER  KEYBOARD_MODIFIER_LEFTCTRL
#define HOTKEY_TOGGLE    HID_KEY_CAPS_LOCK

/**================================================== *
 * ==============  Mouse Speed Factor  ============== *
 * ================================================== *
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

/* Output A values, default is for the most common ~ 16:9 ratio screen */
#define MOUSE_SPEED_A_FACTOR_X 16
#define MOUSE_SPEED_A_FACTOR_Y 28

/* Output B values, default is for the most common ~ 16:9 ratio screen */
#define MOUSE_SPEED_B_FACTOR_X 16
#define MOUSE_SPEED_B_FACTOR_Y 28

#define JUMP_THRESHOLD 0

/* Mouse acceleration */
#define ENABLE_ACCELERATION 1

/**================================================== *
 * ===========  Gaming Edge Switching  ============== *
 * ================================================== *
 *
 * Gaming mode edge switching allows switching outputs in gaming mode
 * by moving the mouse toward the edge of the screen.
 *
 * GAMING_EDGE_THRESHOLD: [0-4294967295], accumulated horizontal movement needed
 *                        to trigger a switch (in mouse movement units)
 * GAMING_EDGE_WINDOW_MS: [0-4294967295], time window in milliseconds for
 *                        accumulating movement (max ~49 days)
 * GAMING_EDGE_MAX_VERTICAL: [0-4294967295], maximum allowed vertical movement
 *                          during edge detection (prevents accidental switches
 *                          when moving mouse diagonally)
 *
 * */

#define GAMING_EDGE_THRESHOLD 20000
#define GAMING_EDGE_WINDOW_MS 1000
#define GAMING_EDGE_MAX_VERTICAL 2000

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
 * */

/**================================================== *
 *
 * SCREENSAVER_{A|B}_MODE: DISABLED
 *                         PONG
 *                         JITTER
 *
 * */

#define SCREENSAVER_A_MODE DISABLED
#define SCREENSAVER_B_MODE DISABLED

/**================================================== *
 *
 * SCREENSAVER_{A|B}_IDLE_TIME_SEC: Number of seconds that an output
 * must be inactive before the screensaver mode will be activated.
 *
 * */

#define SCREENSAVER_A_IDLE_TIME_SEC 240
#define SCREENSAVER_B_IDLE_TIME_SEC 240

/**================================================== *
 *
 * SCREENSAVER_{A|B}_MAX_TIME_SEC: Number of seconds that the screensaver
 * will run on an output before being deactivated. 0 for indefinitely.
 *
 * */

#define SCREENSAVER_A_MAX_TIME_SEC 0
#define SCREENSAVER_B_MAX_TIME_SEC 0

/**================================================== *
 *
 * SCREENSAVER_{A|B}_ONLY_IF_INACTIVE: [0 or 1] 1 means the
 * screensaver will activate only if the output is inactive.
 *
 * */

#define SCREENSAVER_A_ONLY_IF_INACTIVE 0
#define SCREENSAVER_B_ONLY_IF_INACTIVE 0

/**================================================== *
 * ================  Output OS Config =============== *
 * ================================================== *
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

#define OUTPUT_A_OS MACOS
#define OUTPUT_B_OS LINUX


/**================================================== *
 * =================  Enforce Ports ================= *
 * ================================================== *
 *
 * If enabled, fixes some device incompatibilities by
 * enforcing keyboard has to be in port A and mouse in port B.
 *
 * ENFORCE_PORTS: [0, 1] - 1 means keyboard has to plug in A and mouse in B
 *                         0 means no such layout is enforced
 *
 * */

#define ENFORCE_PORTS 0


/**================================================== *
 * =============  Enforce Boot Protocol ============= *
 * ================================================== *
 *
 * If enabled, fixes some device incompatibilities by
 * enforcing the boot protocol (which is simpler to parse
 * and with less variation)
 *
 * ENFORCE_KEYBOARD_BOOT_PROTOCOL: [0, 1] - 1 means keyboard will forcefully use
 *                                          the boot protocol
 *                                        - 0 means no such thing is enforced
 *
 * */

#define ENFORCE_KEYBOARD_BOOT_PROTOCOL 0
