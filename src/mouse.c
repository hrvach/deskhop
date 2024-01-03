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

#include "main.h"

int get_mouse_offset(int32_t movement, const int direction) {
    int offset = 0;

    if (direction == DIRECTION_X)
        offset = movement * MOUSE_SPEED_FACTOR_X;
    else
        offset = movement * MOUSE_SPEED_FACTOR_Y;

    /* Holding a special hotkey enables mouse to slow down as much as possible
       when you need that extra precision */
    if (global_state.mouse_zoom)
        offset = offset >> 2;

    return offset;
}

void keep_cursor_on_screen(int16_t* position, const int32_t* movement, const int direction) {
    int16_t offset = get_mouse_offset(*movement, direction);

    /* Lowest we can go is 0 */
    if (*position + offset < 0)
        *position = 0;

    /* Highest we can go is MAX_SCREEN_COORD */
    else if (*position + offset > MAX_SCREEN_COORD)
        *position = MAX_SCREEN_COORD;

    /* We're still on screen, all good */
    else
        *position += offset;
}


void check_mouse_switch(const mouse_values_t* values, device_state_t* state) {
    hid_abs_mouse_report_t report = {.y = 0, .x = MAX_SCREEN_COORD};
    
    /* No switching allowed if explicitly disabled */
    if (state->switch_lock)
        return;

    /* End of screen left switches screen A->B */
    bool jump_from_A_to_B = (state->mouse_x + values->move_x < -MOUSE_JUMP_THRESHOLD  &&
        state->active_output == ACTIVE_OUTPUT_A);

    /* End of screen right switches screen B->A */
    bool jump_from_B_to_A = (state->mouse_x + values->move_x > MAX_SCREEN_COORD + MOUSE_JUMP_THRESHOLD &&
        state->active_output == ACTIVE_OUTPUT_B);

    if (jump_from_A_to_B || jump_from_B_to_A) {
        /* Hide mouse pointer in the upper right corner on the system we are switching FROM
           If the mouse is locally attached to the current board or notify other board if not */
        if (state->active_output == state->mouse_connected)
            queue_mouse_report(&report, state);                
        else
            send_packet((const uint8_t*)&report, MOUSE_REPORT_MSG, MOUSE_REPORT_LENGTH);

        if (jump_from_A_to_B) {
            switch_output(ACTIVE_OUTPUT_B);
            state->mouse_x = MAX_SCREEN_COORD;
        } else {
            switch_output(ACTIVE_OUTPUT_A);
            state->mouse_x = 0;
        }
    }    
}

void extract_values_report_protocol(uint8_t* report,
                                    device_state_t* state,
                                    mouse_values_t* values) {
    /* If Report ID is used, the report is prefixed by the report ID so we have to move by 1 byte */
    if (state->mouse_dev.uses_report_id) {        
        /* Move past the ID to parse the report */
        report++;
    }

    values->move_x = get_report_value(report, &state->mouse_dev.move_x);
    values->move_y = get_report_value(report, &state->mouse_dev.move_y);
    values->wheel = get_report_value(report, &state->mouse_dev.wheel);
    values->buttons = get_report_value(report, &state->mouse_dev.buttons);

    /* Mice generally come in 3 categories - 8-bit, 12-bit and 16-bit. */
    switch (state->mouse_dev.move_x.size) {
        case 12:
            /* If we're already 12 bit, great! */
            break;
        case 16:
            /* Initially we downscale fancy mice to 12-bits, 
               adding a 32-bit internal coordinate tracking is TODO */
            values->move_x >>= 4;
            values->move_y >>= 4;
            break;
        default:
            /* 8-bit is the default, upscale to 12-bit. */
            values->move_x <<= 4;
            values->move_y <<= 4;
    }
}

void extract_values_boot_protocol(uint8_t* report, device_state_t* state, mouse_values_t* values) {
    hid_mouse_report_t* mouse_report = (hid_mouse_report_t*)report;
    /* For 8-bit values, we upscale them to 12-bit, TODO: 16 bit */
    values->move_x = mouse_report->x << 4;
    values->move_y = mouse_report->y << 4;
    values->wheel = mouse_report->wheel;
    values->buttons = mouse_report->buttons;
}

void process_mouse_report(uint8_t* raw_report, int len, device_state_t* state) {
    mouse_values_t values = {0};

    /* Interpret values depending on the current protocol used */
    if (state->mouse_dev.protocol == HID_PROTOCOL_BOOT)
        extract_values_boot_protocol(raw_report, state, &values);
    else
        extract_values_report_protocol(raw_report, state, &values);

    /* We need to enforce the cursor doesn't go off-screen, that would be bad. */
    keep_cursor_on_screen(&state->mouse_x, &values.move_x, DIRECTION_X);
    keep_cursor_on_screen(&state->mouse_y, &values.move_y, DIRECTION_Y);

    hid_abs_mouse_report_t abs_mouse_report = {
        .buttons = values.buttons,
        .x = state->mouse_x,
        .y = state->mouse_y,
        .wheel = values.wheel,
        .pan = 0
    };

    if (state->active_output == ACTIVE_OUTPUT_A) {
        send_packet((const uint8_t*)&abs_mouse_report, MOUSE_REPORT_MSG, MOUSE_REPORT_LENGTH);
    } else {
        queue_mouse_report(&abs_mouse_report, state);
        state->last_activity[ACTIVE_OUTPUT_B] = time_us_64();
    }

    /* We use the mouse to switch outputs, the logic is in check_mouse_switch() */
    check_mouse_switch(&values, state);
}

/* ==================================================== *
 * Mouse Queue Section
 * ==================================================== */

void process_mouse_queue_task(device_state_t* state) {
    hid_abs_mouse_report_t report = {0};

    /* We need to be connected to the host to send messages */
    if (!state->tud_connected)
        return;

    /* Peek first, if there is anything there... */
    if (!queue_try_peek(&state->mouse_queue, &report))
        return;

    /* ... try sending it to the host, if it's successful */
    bool succeeded = tud_hid_abs_mouse_report(REPORT_ID_MOUSE, report.buttons, report.x, report.y,
                                              report.wheel, report.pan);

    /* ... then we can remove it from the queue */
    if (succeeded)
        queue_try_remove(&state->mouse_queue, &report);
}

void queue_mouse_report(hid_abs_mouse_report_t* report, device_state_t* state) {
    /* It wouldn't be fun to queue up a bunch of messages and then dump them all on host */
    if (!state->tud_connected)
        return;

    queue_try_add(&state->mouse_queue, report);
}
