#ifndef MODE_MANAGER_H
#define MODE_MANAGER_H

#include "user_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// 模式管理器初始化
void mode_manager_init(void);

// 获取当前模式
device_mode_t get_current_mode(void);

// 切换到下一个模式
void switch_to_next_mode(void);

// 切换到指定模式
void switch_to_mode(device_mode_t new_mode);

// 获取模式名称
const char* get_mode_name(device_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif