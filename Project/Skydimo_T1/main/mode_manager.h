#pragma once
#include <stdint.h>

typedef enum {
    MODE_CLOCK = 0,
    MODE_WEATHER,
    MODE_GALLERY,
    MODE_KEYBOARD,
    MODE_CONFIG,
    MODE_MAX
} mode_t;

mode_t mode_get(void);
mode_t mode_next(void);