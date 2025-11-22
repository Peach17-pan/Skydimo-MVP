#pragma once
#include <stdint.h>

typedef enum {
    MODE_CLOCK = 0,    // 时钟模式
    MODE_WEATHER,      // 天气模式  
    MODE_GALLERY,      // 相册模式
    MODE_KEYBOARD,     // 键盘模式
    MODE_CONFIG,       // 配网模式
    MODE_MAX
} mode_t;

mode_t mode_get(void);
mode_t mode_next(void);
void mode_set(mode_t new_mode);  // 新增：设置特定模式