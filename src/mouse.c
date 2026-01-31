/*
 * This file is part of DeskHop (https://github.com/hrvach/deskhop).
 * Copyright (c) 2025 Hrvoje Cavrak
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * See the file LICENSE for the full license text.
 */

#include "main.h"
#include <math.h>

#define MACOS_SWITCH_MOVE_X 10
#define MACOS_SWITCH_MOVE_COUNT 5
#define ACCEL_POINTS 7

void switch_virtual_desktop_macos(device_t *state, int direction);

/* Check if our upcoming mouse movement would result in having to switch outputs */
enum screen_pos_e is_horizontal_switch_needed(int position, int offset) {
    if (position + offset < MIN_SCREEN_COORD - global_state.config.jump_threshold)
        return LEFT;

    if (position + offset > MAX_SCREEN_COORD + global_state.config.jump_threshold)
        return RIGHT;

    return NONE;
}

enum screen_pos_e is_vertical_switch_needed(int position, int offset) {
    if (position + offset < MIN_SCREEN_COORD - global_state.config.jump_threshold)
        return TOP;

    if (position + offset > MAX_SCREEN_COORD + global_state.config.jump_threshold)
        return BOTTOM;

    return NONE;
}

/* Move mouse coordinate 'position' by 'offset', but don't fall off the screen */
int32_t move_and_keep_on_screen(int position, int offset) {
    /* Lowest we can go is 0 */
    if (position + offset < MIN_SCREEN_COORD)
        return MIN_SCREEN_COORD;

    /* Highest we can go is MAX_SCREEN_COORD */
    else if (position + offset > MAX_SCREEN_COORD)
        return MAX_SCREEN_COORD;

    /* We're still on screen, all good */
    return position + offset;
}

/* Implement basic mouse acceleration based on actual 2D movement magnitude.
   Returns the acceleration factor to apply to both x and y components. */
float calculate_mouse_acceleration_factor(int32_t offset_x, int32_t offset_y) {
    const struct curve {
        int value;
        float factor;
    } acceleration[ACCEL_POINTS] = {
                   // 4 |                                        *
        {2, 1},    //   |                                  *
        {5, 1.1},  // 3 |
        {15, 1.4}, //   |                       *
        {30, 1.9}, // 2 |                *
        {45, 2.6}, //   |        *
        {60, 3.4}, // 1 |  *
        {70, 4.0}, //    -------------------------------------------
    };             //        10    20    30    40    50    60    70

    if (offset_x == 0 && offset_y == 0)
        return 1.0;

    if (!global_state.config.enable_acceleration)
        return 1.0;

    // Calculate the 2D movement magnitude
    const float movement_magnitude = sqrtf((float)(offset_x * offset_x) + (float)(offset_y * offset_y));

    if (movement_magnitude <= acceleration[0].value)
        return acceleration[0].factor;

    if (movement_magnitude >= acceleration[ACCEL_POINTS-1].value)
        return acceleration[ACCEL_POINTS-1].factor;

    const struct curve *lower = NULL;
    const struct curve *upper = NULL;

    for (int i = 0; i < ACCEL_POINTS-1; i++) {
        if (movement_magnitude < acceleration[i + 1].value) {
            lower = &acceleration[i];
            upper = &acceleration[i + 1];
            break;
        }
    }

    // Should never happen, but just in case
    if (lower == NULL || upper == NULL)
        return 1.0;

    const float interpolation_pos = (movement_magnitude - lower->value) /
                                  (upper->value - lower->value);

    return lower->factor + interpolation_pos * (upper->factor - lower->factor);
}

/* Returns direction if need to switch screens, NONE otherwise */
enum screen_pos_e update_mouse_position(device_t *state, mouse_values_t *values) {
    output_t *current    = &state->config.output[state->active_output];
    uint8_t reduce_speed = 0;

    /* Check if we are configured to move slowly */
    if (state->mouse_zoom)
        reduce_speed = MOUSE_ZOOM_SCALING_FACTOR;

    /* Calculate movement */
    float acceleration_factor = calculate_mouse_acceleration_factor(values->move_x, values->move_y);
    int offset_x = round(values->move_x * acceleration_factor * (current->speed_x >> reduce_speed));
    int offset_y = round(values->move_y * acceleration_factor * (current->speed_y >> reduce_speed));

    /* Determine if our upcoming movement would stay within the screen */
    enum screen_pos_e horizontal_switch = is_horizontal_switch_needed(state->pointer_x, offset_x);
    enum screen_pos_e vertical_switch = is_vertical_switch_needed(state->pointer_y, offset_y);

    /* Update movement (clamp if no switch is happening) */
    if (horizontal_switch == NONE)
        state->pointer_x = move_and_keep_on_screen(state->pointer_x, offset_x);
    if (vertical_switch == NONE)
        state->pointer_y = move_and_keep_on_screen(state->pointer_y, offset_y);

    /* Update buttons state */
    state->mouse_buttons = values->buttons;

    /* Prioritize horizontal switches */
    if (horizontal_switch != NONE)
        return horizontal_switch;
    return vertical_switch;
}

/* If we are active output, queue packet to mouse queue, else send them through UART */
void output_mouse_report(mouse_report_t *report, device_t *state) {
    if (CURRENT_BOARD_IS_ACTIVE_OUTPUT) {
        queue_mouse_report(report, state);
        state->last_activity[BOARD_ROLE] = time_us_64();
    } else {
        queue_packet((uint8_t *)report, MOUSE_REPORT_MSG, MOUSE_REPORT_LENGTH);
    }
}

/* Map coordinate when transitioning between screens.
 * For horizontal layouts: maps Y coordinate.
 * For vertical layouts: maps X coordinate. */
int16_t map_screen_transition(int pointer, border_size_t *from, border_size_t *to) {
    int size_from = from->end - from->start;
    int size_to = to->end - to->start;

    /* Handle degenerate cases */
    if (size_from <= 0)
        return pointer;

    if (size_to <= 0)
        return to->start;

    /* Linear interpolation from source range to destination range */
    return to->start + ((pointer - from->start) * size_to) / size_from;
}

/* When transitioning to a macOS output with multiple screens, push cursor to
 * screen 1. External events may have moved it to another screen without our
 * knowledge. We assume worst case (cursor on screen_count) and push toward
 * the border. If already on screen 1, macOS ignores the push. */
void reset_macos_to_screen1(device_t *state, output_t *output) {
    if (output->screen_count <= 1)
        return;

    int16_t saved_pointer_y = state->pointer_y;
    int8_t push_direction = (output->pos == LEFT) ? RIGHT : LEFT;

    for (int8_t from_screen = output->screen_count; from_screen > 1; from_screen--) {
        border_size_t *range = &output->screen_transition[from_screen - 2].to;
        state->pointer_y = (range->start < range->end)
                               ? (range->start + range->end) / 2
                               : MAX_SCREEN_COORD / 2;
        switch_virtual_desktop_macos(state, push_direction);
    }

    output->screen_index = 1;
    state->pointer_y = saved_pointer_y;
}

void switch_to_another_pc(device_t *state, output_t *output, int output_to, int direction) {
    /* When monitor and computer orientations differ, only border_monitor_index can switch */
    bool is_vertical_monitor = (output->monitor_layout == LAYOUT_VERTICAL);
    bool is_vertical_computer = (output->pos == TOP || output->pos == BOTTOM);
    if (is_vertical_monitor != is_vertical_computer &&
        output->screen_index != output->border_monitor_index) {
        return;
    }

    screen_transition_t *border = &state->config.computer_border;

    /* Determine source/dest ranges based on which output we're leaving */
    border_size_t *from_range = (state->active_output == 0) ? &border->from : &border->to;
    border_size_t *to_range   = (state->active_output == 0) ? &border->to : &border->from;

    /* Only apply bounds check and mapping if BOTH directions are configured.
     * This allows the user to travel to the other computer to configure the return path. */
    bool from_valid = (from_range->start < from_range->end);
    bool to_valid = (to_range->start < to_range->end);
    bool range_valid = from_valid && to_valid;

    /* Use computer layout to determine which coordinate to check (vertical = X, horizontal = Y) */
    int16_t check_coord = is_vertical_computer ? state->pointer_x : state->pointer_y;

    /* Block transition if cursor is outside the allowed range */
    if (range_valid && (check_coord < from_range->start || check_coord > from_range->end))
        return;

    uint8_t *mouse_park_pos = &state->config.output[state->active_output].mouse_park_pos;

    int16_t mouse_y = (*mouse_park_pos == 0) ? MIN_SCREEN_COORD : /* Top */
                      (*mouse_park_pos == 1) ? MAX_SCREEN_COORD : /* Bottom */
                                               state->pointer_y;  /* Previous */

    mouse_report_t hidden_pointer = {.y = mouse_y, .x = MAX_SCREEN_COORD};

    output_mouse_report(&hidden_pointer, state);
    set_active_output(state, output_to);

    /* For macOS with multiple screens, reset cursor to screen 1 */
    if (state->config.output[output_to].os == MACOS)
        reset_macos_to_screen1(state, &state->config.output[output_to]);

    /* Set new position based on computer layout */
    if (is_vertical_computer) {
        state->pointer_y = (direction == TOP) ? MAX_SCREEN_COORD : MIN_SCREEN_COORD;
        state->pointer_x = map_screen_transition(state->pointer_x, from_range, to_range);
    } else {
        state->pointer_x = (direction == LEFT) ? MAX_SCREEN_COORD : MIN_SCREEN_COORD;
        state->pointer_y = map_screen_transition(state->pointer_y, from_range, to_range);
    }
}

void switch_virtual_desktop_macos(device_t *state, int direction) {
    /*
     * Fix for MACOS: Before sending new absolute report setting coordinate to edge:
     * 1. Move the cursor to the edge of the screen at the current position
     * 2. Send relative mouse movement in the direction of movement to get
     *    the cursor onto the next screen
     */
    bool is_vertical = (direction == TOP || direction == BOTTOM);

    mouse_report_t edge_position = {
        .x = is_vertical ? state->pointer_x : ((direction == LEFT) ? MIN_SCREEN_COORD : MAX_SCREEN_COORD),
        .y = is_vertical ? ((direction == TOP) ? MIN_SCREEN_COORD : MAX_SCREEN_COORD) : state->pointer_y,
        .mode = ABSOLUTE,
        .buttons = state->mouse_buttons,
    };

    int16_t move_amount = MACOS_SWITCH_MOVE_X;
    if (direction == LEFT || direction == TOP)
        move_amount = -move_amount;

    mouse_report_t move_relative_one = {
        .x = is_vertical ? 0 : move_amount,
        .y = is_vertical ? move_amount : 0,
        .mode = RELATIVE,
        .buttons = state->mouse_buttons,
    };

    output_mouse_report(&edge_position, state);

    /* Once doesn't seem reliable enough, do it a few times */
    for (int i = 0; i < MACOS_SWITCH_MOVE_COUNT; i++)
        output_mouse_report(&move_relative_one, state);
}

void switch_virtual_desktop(device_t *state, output_t *output, int new_index, int direction) {
    int current_index = output->screen_index;
    bool is_vertical = (output->monitor_layout == LAYOUT_VERTICAL);

    /* Determine which transition we're using (screen_index is 1-based) */
    int transition_idx;
    border_size_t *allowed_range;
    border_size_t *target_range;
    screen_transition_t *transition;

    if (new_index > current_index) {
        /* Moving to higher screen index (e.g., 1→2 or 2→3) */
        transition_idx = current_index - 1;
        transition = &output->screen_transition[transition_idx];
        allowed_range = &transition->from;
        target_range = &transition->to;
    } else {
        /* Moving to lower screen index (e.g., 2→1 or 3→2) */
        transition_idx = new_index - 1;
        transition = &output->screen_transition[transition_idx];
        allowed_range = &transition->to;
        target_range = &transition->from;
    }

    /* Only apply bounds check and mapping if BOTH directions of this transition are configured.
     * This allows the user to travel to the other screen to configure the return path. */
    bool from_valid = (transition->from.start < transition->from.end);
    bool to_valid = (transition->to.start < transition->to.end);
    bool range_valid = from_valid && to_valid;

    /* Check the appropriate coordinate based on layout */
    int16_t check_coord = is_vertical ? state->pointer_x : state->pointer_y;

    /* Block transition if cursor is outside the allowed range */
    if (range_valid && (check_coord < allowed_range->start || check_coord > allowed_range->end))
        return;

    switch (output->os) {
        case MACOS:
            switch_virtual_desktop_macos(state, direction);
            break;

        case WINDOWS:
            /* TODO: Switch to relative-only if index > 1, but keep tabs to switch back */
            state->relative_mouse = (new_index > 1);
            break;

        case LINUX:
        case ANDROID:
        case OTHER:
            /* Linux should treat all desktops as a single virtual screen, so you should leave
            screen_count at 1 and it should just work */
            break;
    }

    /* Map coordinate to destination range after OS handler positions cursor at screen edge */
    if (is_vertical) {
        if (range_valid)
            state->pointer_x = map_screen_transition(state->pointer_x, allowed_range, target_range);
        state->pointer_y = (direction == BOTTOM) ? MIN_SCREEN_COORD : MAX_SCREEN_COORD;
    } else {
        if (range_valid)
            state->pointer_y = map_screen_transition(state->pointer_y, allowed_range, target_range);
        state->pointer_x = (direction == RIGHT) ? MIN_SCREEN_COORD : MAX_SCREEN_COORD;
    }

    output->screen_index = new_index;
}

/*                               BORDER
                                   |
       .---------.    .---------.  |  .---------.    .---------.    .---------.
      ||    B2   ||  ||    B1   || | ||    A1   ||  ||    A2   ||  ||    A3   ||   (output, index)
      ||  extra  ||  ||   main  || | ||   main  ||  ||  extra  ||  ||  extra  ||   (main or extra)
       '---------'    '---------'  |  '---------'    '---------'    '---------'
          )___(          )___(     |     )___(          )___(          )___(
*/
void do_screen_switch(device_t *state, int direction) {
    output_t *output = &state->config.output[state->active_output];

    /* No switching allowed if explicitly disabled or in gaming mode */
    if (state->switch_lock || state->gaming_mode)
        return;

    bool is_vertical_monitor = (output->monitor_layout == LAYOUT_VERTICAL);
    /* Derive computer layout from position (TOP/BOTTOM = vertical, LEFT/RIGHT = horizontal) */
    bool is_vertical_computer = (output->pos == TOP || output->pos == BOTTOM);
    bool dir_is_vertical = (direction == TOP || direction == BOTTOM);

    /* Is direction along the monitor layout? (could move between monitors) */
    bool dir_along_monitors = (is_vertical_monitor == dir_is_vertical);

    /* Is direction toward the other computer? Only true if direction matches computer orientation
     * and we're moving opposite to our position (toward the border where the other computer is) */
    bool toward_other_computer = (is_vertical_computer == dir_is_vertical) && (output->pos != direction);

    if (dir_along_monitors) {
        /* Movement along monitor layout (between screens on same computer) */
        int original_screen_index = output->screen_index;

        /* Are we moving toward screen 1 (the border screen)?
         * Screen 1 is always closest to the computer border (opposite of pos).
         * When monitor and computer orientations match, this equals toward_other_computer.
         * When they differ, use geometric convention (LEFT/TOP = toward lower index). */
        bool toward_screen_1;
        if (is_vertical_monitor == is_vertical_computer) {
            toward_screen_1 = toward_other_computer;
        } else {
            toward_screen_1 = is_vertical_monitor ? (direction == TOP) : (direction == LEFT);
        }

        if (toward_screen_1) {
            if (output->screen_index > 1) {
                /* Move to lower screen index */
                switch_virtual_desktop(state, output, output->screen_index - 1, direction);
            } else if (toward_other_computer) {
                /* At screen 1 AND moving toward other computer - try to switch */
                if (state->mouse_buttons)
                    return;
                switch_to_another_pc(state, output, 1 - state->active_output, direction);
                return;
            }
        } else if (output->screen_index < output->screen_count) {
            /* Move to higher screen index */
            switch_virtual_desktop(state, output, output->screen_index + 1, direction);
        }

        /* If screen didn't change, clamp position */
        if (output->screen_index == original_screen_index) {
            if (is_vertical_monitor)
                state->pointer_y = (direction == TOP) ? MIN_SCREEN_COORD : MAX_SCREEN_COORD;
            else
                state->pointer_x = (direction == LEFT) ? MIN_SCREEN_COORD : MAX_SCREEN_COORD;
        }
    } else {
        /* Movement perpendicular to monitor layout (orientations differ) */
        if (toward_other_computer) {
            /* Only the configured border monitor can switch to other computer */
            bool can_switch = (output->screen_index == output->border_monitor_index);

            if (can_switch) {
                if (state->mouse_buttons)
                    return;
                switch_to_another_pc(state, output, 1 - state->active_output, direction);
                return;
            }
        }

        /* Clamp position */
        if (dir_is_vertical)
            state->pointer_y = (direction == TOP) ? MIN_SCREEN_COORD : MAX_SCREEN_COORD;
        else
            state->pointer_x = (direction == LEFT) ? MIN_SCREEN_COORD : MAX_SCREEN_COORD;
    }
}

static inline bool extract_value(bool uses_id, int32_t *dst, report_val_t *src, uint8_t *raw_report, int len) {
    /* If HID Report ID is used, the report is prefixed by the report ID so we have to move by 1 byte */
    if (uses_id && (*raw_report++ != src->report_id))
        return false;

    *dst = get_report_value(raw_report, len, src);
    return true;
}

void extract_report_values(uint8_t *raw_report, int len, device_t *state, mouse_values_t *values, hid_interface_t *iface) {
    /* Interpret values depending on the current protocol used. */
    if (iface->protocol == HID_PROTOCOL_BOOT) {
        hid_mouse_report_t *mouse_report = (hid_mouse_report_t *)raw_report;

        values->move_x  = mouse_report->x;
        values->move_y  = mouse_report->y;
        values->wheel   = mouse_report->wheel;
        values->pan     = mouse_report->pan;
        values->buttons = mouse_report->buttons;
        return;
    }
    mouse_t *mouse = &iface->mouse;
    bool uses_id = iface->uses_report_id;

    extract_value(uses_id, &values->move_x, &mouse->move_x, raw_report, len);
    extract_value(uses_id, &values->move_y, &mouse->move_y, raw_report, len);
    extract_value(uses_id, &values->wheel, &mouse->wheel, raw_report, len);
    extract_value(uses_id, &values->pan, &mouse->pan, raw_report, len);

    if (!extract_value(uses_id, &values->buttons, &mouse->buttons, raw_report, len)) {
        values->buttons = state->mouse_buttons;
    }
}

mouse_report_t create_mouse_report(device_t *state, mouse_values_t *values) {
    mouse_report_t mouse_report = {
        .buttons = values->buttons,
        .x       = state->pointer_x,
        .y       = state->pointer_y,
        .wheel   = values->wheel,
        .pan     = values->pan,
        .mode    = ABSOLUTE,
    };

    /* Workaround for Windows multiple desktops */
    if (state->relative_mouse || state->gaming_mode) {
        mouse_report.x = values->move_x;
        mouse_report.y = values->move_y;
        mouse_report.mode = RELATIVE;
    }

    return mouse_report;
}

void process_mouse_report(uint8_t *raw_report, int len, uint8_t itf, hid_interface_t *iface) {
    mouse_values_t values = {0};
    device_t *state = &global_state;

    /* Interpret the mouse HID report, extract and save values we need. */
    extract_report_values(raw_report, len, state, &values, iface);

    /* Calculate and update mouse pointer movement. */
    enum screen_pos_e switch_direction = update_mouse_position(state, &values);

    /* Create the report for the output PC based on the updated values */
    mouse_report_t report = create_mouse_report(state, &values);

    /* Move the mouse, depending where the output is supposed to go */
    output_mouse_report(&report, state);

    /* We use the mouse to switch outputs, if switch_direction is not NONE */
    if (switch_direction != NONE)
        do_screen_switch(state, switch_direction);
}

/* ==================================================== *
 * Mouse Queue Section
 * ==================================================== */

void process_mouse_queue_task(device_t *state) {
    mouse_report_t report = {0};

    /* We need to be connected to the host to send messages */
    if (!state->tud_connected)
        return;

    /* Peek first, if there is anything there... */
    if (!queue_try_peek(&state->mouse_queue, &report))
        return;

    /* If we are suspended, let's wake the host up */
    if (tud_suspended())
        tud_remote_wakeup();

    /* Check interface readiness. In ABSOLUTE mode, we send to both interfaces
     * (buttons via relative, position via absolute), so both must be ready. */
    if (report.mode == ABSOLUTE) {
        if (!tud_hid_n_ready(ITF_NUM_HID) || !tud_hid_n_ready(ITF_NUM_HID_REL_M))
            return;
    } else {
        if (!tud_hid_n_ready(ITF_NUM_HID_REL_M))
            return;
    }

    /* Try sending it to the host, if it's successful */
    bool succeeded
        = tud_mouse_report(report.mode, report.buttons, report.x, report.y, report.wheel, report.pan);

    /* ... then we can remove it from the queue */
    if (succeeded)
        queue_try_remove(&state->mouse_queue, &report);
}

void queue_mouse_report(mouse_report_t *report, device_t *state) {
    /* It wouldn't be fun to queue up a bunch of messages and then dump them all on host */
    if (!state->tud_connected)
        return;

    queue_try_add(&state->mouse_queue, report);
}
