#ifndef MODE_MANAGER_H
#define MODE_MANAGER_H

#include "lvgl.h"

// 设备模式枚举
typedef enum {
    MODE_NETWORK_CONFIG = 0,
    MODE_CLOCK,
    MODE_WEATHER,
    MODE_GALLERY,
    MODE_VIRTUAL_KEYBOARD,
    MODE_COUNT
} device_mode_t;

// 函数声明
void mode_manager_init(void);
void mode_switch_next(void);
void mode_switch_to(device_mode_t mode);
device_mode_t get_current_mode(void);
const char* get_mode_name(device_mode_t mode);
void mode_manager_task(void *arg);

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif