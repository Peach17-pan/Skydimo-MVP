#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// 模式定义
typedef enum {
    MODE_WIFI = 0,
    MODE_CLOCK,
    MODE_WEATHER,
    MODE_GALLERY,
    MODE_KEYBOARD,
    MODE_MAX
} Mode;

// 事件类型定义
typedef enum {
    EVENT_BUTTON_SHORT_PRESS = 0,
    EVENT_BUTTON_LONG_PRESS,
    EVENT_TOUCH,
    EVENT_ROTARY
} event_type_t;

// 事件结构
typedef struct {
    event_type_t type;
    uint32_t duration;
    uint16_t x;
    uint16_t y;
} input_event_t;

// 系统句柄结构
typedef struct {
    esp_lcd_panel_handle_t panel_handle;
    bool display_initialized;
    Mode current_mode;
    TickType_t mode_start_time;
    QueueHandle_t event_queue;
} system_handle_t;

#endif