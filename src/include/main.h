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

#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <pico/util/queue.h>
#include "hid_parser.h"

#include "constants.h"
#include "misc.h"
#include "structs.h"
#include "config.h"

#include "pio_usb.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "user_config.h"
#include "protocol.h"

#include "dma.h"

#include "firmware.h"
#include "flash.h"
#include "handlers.h"
#include "keyboard.h"
#include "mouse.h"
#include "packet.h"
#include "pinout.h"
#include "screen.h"
#include "serial.h"
#include "setup.h"
#include "tasks.h"
#include "watchdog.h"


#include <hardware/structs/ioqspi.h>
#include <hardware/structs/sio.h>
#include <hardware/dma.h>
#include <hardware/flash.h>
#include <hardware/sync.h>
#include <hardware/watchdog.h>
#include <pico/bootrom.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <pico/unique_id.h>
