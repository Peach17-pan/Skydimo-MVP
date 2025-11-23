#include "key_detector.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <stdbool.h>

static const char *TAG = "key_detector";

static key_event_callback_t short_press_callback = NULL;
static key_event_callback_t long_press_callback = NULL;
static TimerHandle_t long_press_timer = NULL;
static bool key_pressed = false;
static uint32_t key_press_start_time = 0;

// 长按定时器回调
static void long_press_timer_callback(TimerHandle_t xTimer)
{
    if (key_pressed && long_press_callback) {
        ESP_LOGI(TAG, "Long press detected");
        long_press_callback();
    }
}

// GPIO中断处理函数 - 简化版本
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    int level = gpio_get_level(EXAMPLE_PIN_NUM_KEY);
    uint32_t current_time = xTaskGetTickCountFromISR() * portTICK_PERIOD_MS;
    
    if (level == 0) { // 按键按下（低电平）
        if (!key_pressed) {
            key_pressed = true;
            key_press_start_time = current_time;
            // 启动长按定时器
            xTimerStartFromISR(long_press_timer, NULL);
        }
    } else { // 按键释放
        if (key_pressed) {
            key_pressed = false;
            xTimerStopFromISR(long_press_timer, NULL);
            
            // 计算按下时间
            uint32_t press_duration = current_time - key_press_start_time;
            
            if (press_duration >= KEY_SHORT_PRESS_TIME_MS && 
                press_duration < KEY_LONG_PRESS_TIME_MS) {
                // 短按事件
                if (short_press_callback) {
                    ESP_LOGI(TAG, "Short press detected, duration: %lu ms", (unsigned long)press_duration);
                    // 不在ISR中直接调用，通过标志位处理
                    short_press_callback();
                }
            }
        }
    }
}

void key_detector_init(key_event_callback_t short_press_cb, 
                      key_event_callback_t long_press_cb)
{
    short_press_callback = short_press_cb;
    long_press_callback = long_press_cb;
    
    // 配置GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << EXAMPLE_PIN_NUM_KEY),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&io_conf);
    
    // 创建长按定时器
    long_press_timer = xTimerCreate("long_press_timer", 
                                   pdMS_TO_TICKS(KEY_LONG_PRESS_TIME_MS),
                                   pdFALSE, NULL, long_press_timer_callback);
    
    // 安装GPIO中断服务
    gpio_install_isr_service(0);
    gpio_isr_handler_add(EXAMPLE_PIN_NUM_KEY, gpio_isr_handler, NULL);
    
    ESP_LOGI(TAG, "Key detector initialized on GPIO %d", EXAMPLE_PIN_NUM_KEY);
}

bool is_key_pressed(void)
{
    return key_pressed;
}