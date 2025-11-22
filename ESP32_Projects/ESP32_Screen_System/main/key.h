#pragma once
#include <stdint.h>

typedef enum {
    MODE_NET_CFG = 0,
    MODE_CLOCK,
    MODE_WEATHER,
    MODE_GALLERY,
    MODE_VKEYBOARD,
    MODE_MAX
} sys_mode_t;

void key_init(void);
sys_mode_t key_get_mode(void);