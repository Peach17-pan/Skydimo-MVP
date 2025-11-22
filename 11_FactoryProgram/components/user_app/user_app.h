#ifndef USER_APP_H
#define USER_APP_H

#include "freertos/FreeRTOS.h"
#include "user_config.h"

typedef struct 
{
  int16_t x;
  int16_t y;
}app_touch_t;

extern QueueHandle_t app_touch_data_queue;

#ifdef __cplusplus
extern "C" {
#endif

void user_app_init(void);

// 新增：模式切换函数
void user_app_switch_mode(device_mode_t mode);

// 新增：获取当前模式
device_mode_t user_app_get_current_mode(void);

#ifdef __cplusplus
}
#endif

#endif