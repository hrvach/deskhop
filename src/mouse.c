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
  output_t *active_output =
      &global_state.config.output[global_state.active_output];

  if (direction == DIRECTION_X)
    offset = movement * active_output->speed_x;
  else
    offset = movement * active_output->speed_y;

  /* Holding a special hotkey enables mouse to slow down as much as possible
     when you need that extra precision */
  if (global_state.mouse_zoom)
    offset = offset >> 2;

  return offset;
}

void keep_cursor_on_screen(int16_t *position, const int32_t *movement,
                           const int direction) {
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

/* If mouse needs to go locally, queue packet to mouse queue, else send them
 * through UART */
void send_mouse(hid_abs_mouse_report_t *report, device_state_t *state) {
  if (state->active_output == BOARD_ROLE) {
    queue_mouse_report(report, state);
    state->last_activity[BOARD_ROLE] = time_us_64();
  } else {
    send_packet((uint8_t *)report, MOUSE_REPORT_MSG, MOUSE_REPORT_LENGTH);
  }
}

int16_t scale_y_coord(output_t *from, output_t *to, device_state_t *state) {
  int size_to = to->border.bottom - to->border.top;
  int size_from = from->border.bottom - from->border.top;

  /* If sizes match, there is nothing to do */
  if (size_from == size_to)
    return state->mouse_y;

  /* Moving from bigger ==> smaller screen */
  if (size_from < size_to) {
    if (state->mouse_y < from->border.top)
      return 0;

    if (state->mouse_y > from->border.bottom)
      return MAX_SCREEN_COORD;

    /* y_b = ((y_a - top) * HEIGHT) / (bottom - top) */
    return ((state->mouse_y - from->border.top) * MAX_SCREEN_COORD) /
           size_from;
  }

  /* Moving from smaller ==> bigger screen, 
     y_a = top + (((bottom - top) * y_b) / HEIGHT) */
  return to->border.top + ((size_to * state->mouse_y) / MAX_SCREEN_COORD);  
}

void check_mouse_switch(const mouse_values_t *values, device_state_t *state) {
  hid_abs_mouse_report_t report = {.y = 0, .x = MAX_SCREEN_COORD};

  /* No switching allowed if explicitly disabled */
  if (state->switch_lock)
    return;

  /* End of screen left switches screen A->B */
  bool jump_from_A_to_B =
      (state->mouse_x + values->move_x < -MOUSE_JUMP_THRESHOLD &&
       state->active_output == ACTIVE_OUTPUT_A);

  /* End of screen right switches screen B->A */
  bool jump_from_B_to_A = (state->mouse_x + values->move_x >
                               MAX_SCREEN_COORD + MOUSE_JUMP_THRESHOLD &&
                           state->active_output == ACTIVE_OUTPUT_B);

  if (jump_from_A_to_B || jump_from_B_to_A) {
    /* Hide mouse pointer in the upper right corner on the system we are
       switching FROM If the mouse is locally attached to the current board or
       notify other board if not */
    send_mouse(&report, state);

    if (jump_from_A_to_B) {
      switch_output(ACTIVE_OUTPUT_B);
      state->mouse_x = MAX_SCREEN_COORD;

      state->mouse_y = scale_y_coord(&state->config.output[ACTIVE_OUTPUT_A],
                                     &state->config.output[ACTIVE_OUTPUT_B], 
                                     state);

    } else {
      switch_output(ACTIVE_OUTPUT_A);
      state->mouse_x = 0;

      state->mouse_y = scale_y_coord(&state->config.output[ACTIVE_OUTPUT_B],
                                     &state->config.output[ACTIVE_OUTPUT_A], 
                                     state);
    }
  }
}

void extract_values_report_protocol(uint8_t *report, device_state_t *state,
                                    mouse_values_t *values) {
  /* If Report ID is used, the report is prefixed by the report ID so we have to
   * move by 1 byte */
  if (state->mouse_dev.uses_report_id) {
    /* Move past the ID to parse the report */
    report++;
  }

  values->move_x = get_report_value(report, &state->mouse_dev.move_x);
  values->move_y = get_report_value(report, &state->mouse_dev.move_y);
  values->wheel = get_report_value(report, &state->mouse_dev.wheel);
  values->buttons = get_report_value(report, &state->mouse_dev.buttons);
}

void extract_values_boot_protocol(uint8_t *report, device_state_t *state,
                                  mouse_values_t *values) {
  hid_mouse_report_t *mouse_report = (hid_mouse_report_t *)report;

  values->move_x = mouse_report->x;
  values->move_y = mouse_report->y;
  values->wheel = mouse_report->wheel;
  values->buttons = mouse_report->buttons;
}

void process_mouse_report(uint8_t *raw_report, int len, device_state_t *state) {
  mouse_values_t values = {0};

  /* Interpret values depending on the current protocol used */
  if (state->mouse_dev.protocol == HID_PROTOCOL_BOOT)
    extract_values_boot_protocol(raw_report, state, &values);
  else
    extract_values_report_protocol(raw_report, state, &values);

  /* We need to enforce the cursor doesn't go off-screen, that would be bad. */
  keep_cursor_on_screen(&state->mouse_x, &values.move_x, DIRECTION_X);
  keep_cursor_on_screen(&state->mouse_y, &values.move_y, DIRECTION_Y);

  hid_abs_mouse_report_t abs_mouse_report = {.buttons = values.buttons,
                                             .x = state->mouse_x,
                                             .y = state->mouse_y,
                                             .wheel = values.wheel,
                                             .pan = 0};

  /* Move the mouse, depending where the output is supposed to go */
  send_mouse(&abs_mouse_report, state);

  /* We use the mouse to switch outputs, the logic is in check_mouse_switch() */
  check_mouse_switch(&values, state);
}

/* ==================================================== *
 * Mouse Queue Section
 * ==================================================== */

void process_mouse_queue_task(device_state_t *state) {
  hid_abs_mouse_report_t report = {0};

  /* We need to be connected to the host to send messages */
  if (!state->tud_connected)
    return;

  /* Peek first, if there is anything there... */
  if (!queue_try_peek(&state->mouse_queue, &report))
    return;

  /* If we are suspended, let's wake the host up */
  if (tud_suspended())
    tud_remote_wakeup();

  /* ... try sending it to the host, if it's successful */
  bool succeeded =
      tud_hid_abs_mouse_report(REPORT_ID_MOUSE, report.buttons, report.x,
                               report.y, report.wheel, report.pan);

  /* ... then we can remove it from the queue */
  if (succeeded)
    queue_try_remove(&state->mouse_queue, &report);
}

void queue_mouse_report(hid_abs_mouse_report_t *report, device_state_t *state) {
  /* It wouldn't be fun to queue up a bunch of messages and then dump them all
   * on host */
  if (!state->tud_connected)
    return;

  queue_try_add(&state->mouse_queue, report);
}
