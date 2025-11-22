#ifndef MODE_MANAGER_H
#define MODE_MANAGER_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "user_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// 模式切换事件
typedef struct {
    device_mode_t old_mode;
    device_mode_t new_mode;
} mode_change_event_t;

// 初始化模式管理器
void mode_manager_init(void);

// 获取当前模式
device_mode_t get_current_mode(void);

// 切换到下一个模式
void switch_to_next_mode(void);

// 直接切换到指定模式
void switch_to_mode(device_mode_t mode);

// 获取模式切换事件队列句柄
QueueHandle_t get_mode_change_queue(void);

#ifdef __cplusplus
}
#endif

#endif