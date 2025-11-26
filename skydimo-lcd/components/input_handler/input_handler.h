#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "ui_manager.h"  // 现在 ui_manager.h 包含了所有必要的定义

// 引脚定义
#define BUTTON_PIN         0   // BOOT按钮
#define LED_PIN            48  // 状态指示灯
#define TOUCH_INT_PIN      7   // 触摸中断引脚
#define LONG_PRESS_DURATION 2000 // 长按时间2秒

// 输入处理函数
esp_err_t input_handler_init(system_handle_t *sys_handle);
void IRAM_ATTR input_button_isr_handler(void *arg);
void input_touch_isr_handler(void *arg);
void input_led_task(void *pvParameters);

#endif