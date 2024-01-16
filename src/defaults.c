#include "main.h"

/* Default configuration */
const config_t default_config = {
    .magic_header = 0x0B00B1E5,
    .output[ACTIVE_OUTPUT_A] =
        {
            .number = ACTIVE_OUTPUT_A,
            .speed_x = MOUSE_SPEED_A_FACTOR_X,
            .speed_y = MOUSE_SPEED_A_FACTOR_Y,
            .border = {
                .top = 0,
                .bottom = MAX_SCREEN_COORD,
            },
            .screen_count = 1,
            .screen_index = 0,
        },
    .output[ACTIVE_OUTPUT_B] =
        {
            .number = ACTIVE_OUTPUT_B,
            .speed_x = MOUSE_SPEED_B_FACTOR_X,
            .speed_y = MOUSE_SPEED_B_FACTOR_Y,
            .border = {
                .top = 0,
                .bottom = MAX_SCREEN_COORD,
            },
            .screen_count = 1,
            .screen_index = 0,
        },
};