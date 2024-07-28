#pragma once

#include "main.h"

typedef enum {
    UINT8 = 0,
    UINT16 = 1,
    UINT32 = 2,
    UINT64 = 3,
    INT8 = 4,
    INT16 = 5,
    INT32 = 6,
    INT64 = 7,
    BOOL  = 8
} type_e;

typedef struct {
    uint32_t idx;
    bool readonly;
    type_e type;                         
    uint32_t len;
    size_t offset;
} field_map_t;
