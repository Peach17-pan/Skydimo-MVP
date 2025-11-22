#ifndef BUTTON_BSP_H
#define BUTTON_BSP_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

// 按键事件类型
typedef enum {
    BUTTON_SHORT_PRESS,
    BUTTON_LONG_PRESS
} button_event_t;

// 按键初始化
void button_init(void);

// 获取按键事件队列句柄
QueueHandle_t get_button_event_queue(void);

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif