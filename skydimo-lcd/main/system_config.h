// system_config.h
#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "lvgl.h"

// SPI配置
#define LCD_HOST SPI3_HOST
#define EXAMPLE_PIN_NUM_LCD_CS            (GPIO_NUM_9)
#define EXAMPLE_PIN_NUM_LCD_PCLK          (GPIO_NUM_10) 
#define EXAMPLE_PIN_NUM_LCD_DATA0         (GPIO_NUM_11)
#define EXAMPLE_PIN_NUM_LCD_DATA1         (GPIO_NUM_12)
#define EXAMPLE_PIN_NUM_LCD_DATA2         (GPIO_NUM_13)
#define EXAMPLE_PIN_NUM_LCD_DATA3         (GPIO_NUM_14)
#define EXAMPLE_PIN_NUM_LCD_RST           (GPIO_NUM_21)
#define EXAMPLE_PIN_NUM_BK_LIGHT          (GPIO_NUM_8)

// 屏幕分辨率
#define EXAMPLE_LCD_H_RES              172
#define EXAMPLE_LCD_V_RES              640

// LVGL配置
#define LVGL_TICK_PERIOD_MS    2
#define LVGL_TASK_STACK_SIZE   (8 * 1024)
#define LVGL_TASK_PRIORITY     2

// 系统模式定义
typedef enum {
    MODE_WIFI = 0,      // 配网模式
    MODE_CLOCK,         // 时钟模式  
    MODE_WEATHER,       // 天气模式
    MODE_GALLERY,       // 相册模式
    MODE_KEYBOARD,      // 虚拟键盘模式
    MODE_MAX
} system_mode_t;

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
    system_mode_t current_mode;
    TickType_t mode_start_time;
    QueueHandle_t event_queue;
    lv_display_t *lv_display;
    lv_indev_t *touch_indev;
} system_handle_t;

#endif