#include "main.h"

int get_mouse_offset(int8_t movement) {
    // Holding a special hotkey enables mouse to slow down as much as possible
    // when you need that extra precision
    if (global_state.mouse_zoom)
        return movement * MOUSE_SPEED_FACTOR >> 2;
    else
        return movement * MOUSE_SPEED_FACTOR;
}

void keep_cursor_on_screen(int16_t* position, const int8_t* movement) {
    int16_t offset = get_mouse_offset(*movement);

    // Lowest we can go is 0
    if (*position + offset < 0)
        *position = 0;

    // Highest we can go is MAX_SCREEN_COORD
    else if (*position + offset > MAX_SCREEN_COORD)
        *position = MAX_SCREEN_COORD;

    // We're still on screen, all good
    else
        *position += offset;
}

void check_mouse_switch(const hid_mouse_report_t* mouse_report, device_state_t* state) {
    // End of screen right switches screen B->A
    if ((state->mouse_x + mouse_report->x) > MAX_SCREEN_COORD &&
        state->active_output == ACTIVE_OUTPUT_B) {
        state->mouse_x = 0;
        switch_output(ACTIVE_OUTPUT_A);
        return;
    }

    // End of screen left switches screen A->B
    if ((state->mouse_x + mouse_report->x) < 0 && state->active_output == ACTIVE_OUTPUT_A) {
        state->mouse_x = MAX_SCREEN_COORD;
        switch_output(ACTIVE_OUTPUT_B);
        return;
    }
}

void process_mouse_report(uint8_t* raw_report, int len, device_state_t* state) {
    hid_mouse_report_t* mouse_report = (hid_mouse_report_t*)raw_report;

    // We need to enforce the cursor doesn't go off-screen, that would be bad.
    keep_cursor_on_screen(&state->mouse_x, &mouse_report->x);
    keep_cursor_on_screen(&state->mouse_y, &mouse_report->y);

    if (state->active_output == ACTIVE_OUTPUT_A) {
        hid_abs_mouse_report_t abs_mouse_report;

        abs_mouse_report.buttons = mouse_report->buttons;
        abs_mouse_report.x = state->mouse_x;
        abs_mouse_report.y = state->mouse_y;
        abs_mouse_report.wheel = mouse_report->wheel;
        abs_mouse_report.pan = 0;

        send_packet((const uint8_t*)&abs_mouse_report, MOUSE_REPORT_MSG, MOUSE_REPORT_LENGTH);

    } else {
        tud_hid_abs_mouse_report(REPORT_ID_MOUSE, mouse_report->buttons, state->mouse_x,
                                 state->mouse_y, mouse_report->wheel, 0);

        state->last_activity[ACTIVE_OUTPUT_B] = time_us_64();
    }

    // We use the mouse to switch outputs, the logic is in check_mouse_switch()
    check_mouse_switch(mouse_report, state);
}