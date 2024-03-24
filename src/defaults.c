#include "main.h"

/* Default configuration */
const config_t default_config = {
    .magic_header = 0xB00B1E5,
    .version = CURRENT_CONFIG_VERSION,
    .output[OUTPUT_A] =
        {
            .number = OUTPUT_A,
            .speed_x = MOUSE_SPEED_A_FACTOR_X,
            .speed_y = MOUSE_SPEED_A_FACTOR_Y,
            .border = {
                .top = 0,
                .bottom = MAX_SCREEN_COORD,
            },
            .screen_count = 1,
            .screen_index = 1,
            .os = OUTPUT_A_OS,
            .pos = RIGHT,
            .screensaver = {
                .enabled = SCREENSAVER_A_ENABLED,
                .only_if_inactive = SCREENSAVER_A_ONLY_IF_INACTIVE,
                .idle_time_us = SCREENSAVER_A_IDLE_TIME_SEC * 1000000,
                .max_time_us = SCREENSAVER_A_MAX_TIME_SEC * 1000000,                
            }
        },        
    .output[OUTPUT_B] =
        {
            .number = OUTPUT_B,
            .speed_x = MOUSE_SPEED_B_FACTOR_X,
            .speed_y = MOUSE_SPEED_B_FACTOR_Y,
            .border = {
                .top = 0,
                .bottom = MAX_SCREEN_COORD,
            },
            .screen_count = 1,
            .screen_index = 1,
            .os = OUTPUT_B_OS,
            .pos = LEFT,
            .screensaver = {
                .enabled = SCREENSAVER_B_ENABLED,
                .only_if_inactive = SCREENSAVER_B_ONLY_IF_INACTIVE,
                .idle_time_us = SCREENSAVER_B_IDLE_TIME_SEC * 1000000,
                .max_time_us = SCREENSAVER_B_MAX_TIME_SEC * 1000000,                
            }
        },
};