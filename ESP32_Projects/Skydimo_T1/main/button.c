#include "button.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#define GPIO_BTN     0
#define DEBOUNCE_MS  20
#define LONG_PRESS_MS 2000  // 长按时间改为2秒，为任务2做准备

static uint32_t press_start_time = 0;
static bool button_pressed = false;

void button_init(void)
{
    gpio_config_t io = {
        .pin_bit_mask = BIT64(GPIO_BTN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);
}

int button_scan(void)
{
    int current_level = gpio_get_level(GPIO_BTN);
    
    if (current_level == 0 && !button_pressed) {
        // 检测到按下
        button_pressed = true;
        press_start_time = esp_timer_get_time() / 1000; // 转换为毫秒
        return 0;
    }
    else if (current_level == 1 && button_pressed) {
        // 检测到释放
        button_pressed = false;
        uint32_t press_duration = (esp_timer_get_time() / 1000) - press_start_time;
        
        if (press_duration < DEBOUNCE_MS) {
            return 0; // 抖动，忽略
        }
        else if (press_duration >= LONG_PRESS_MS) {
            return 2; // 长按
        }
        else {
            return 1; // 短按
        }
    }
    
    return 0;
}