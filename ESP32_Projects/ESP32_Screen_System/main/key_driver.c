#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "key_driver.h"

static const char *TAG = "KEY_DRIVER";

static sys_mode_t current_mode = MODE_NET_CFG;
static mode_change_cb_t mode_change_callback = NULL;

void key_driver_init(mode_change_cb_t callback)
{
    // 保存回调函数
    mode_change_callback = callback;
    
    // 配置GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << KEY_GPIO_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    
    ESP_LOGI(TAG, "Key driver initialized on GPIO %d", KEY_GPIO_PIN);
}

void key_driver_task(void *arg)
{
    // 状态机定义
    enum {
        KEY_STATE_IDLE,
        KEY_STATE_PRESSED,
        KEY_STATE_LONG_PRESS
    };
    
    uint8_t key_state = KEY_STATE_IDLE;
    uint32_t press_start_time = 0;
    bool last_key_level = true;  // 上拉状态
    
    ESP_LOGI(TAG, "Key task started");
    
    while (1) {
        bool current_level = gpio_get_level(KEY_GPIO_PIN);  // 0=按下, 1=释放
        
        switch (key_state) {
            case KEY_STATE_IDLE:
                // 检测按下事件（下降沿）
                if (!current_level && last_key_level) {
                    press_start_time = esp_timer_get_time() / 1000;
                    key_state = KEY_STATE_PRESSED;
                    ESP_LOGD(TAG, "Key pressed");
                }
                break;
                
            case KEY_STATE_PRESSED:
                if (current_level) {
                    // 按键已释放
                    uint32_t press_duration = (esp_timer_get_time() / 1000) - press_start_time;
                    
                    // 消抖检查：必须大于DEBOUNCE_MS才认为是有效按键
                    if (press_duration >= DEBOUNCE_MS && press_duration < LONG_PRESS_MS) {
                        // 短按：循环切换模式
                        current_mode = (current_mode + 1) % MODE_MAX;
                        ESP_LOGI(TAG, "Short press detected, switch to mode %d", current_mode);
                        
                        // 调用回调通知模式变化
                        if (mode_change_callback) {
                            mode_change_callback(current_mode);
                        }
                    }
                    key_state = KEY_STATE_IDLE;
                } else if ((esp_timer_get_time() / 1000) - press_start_time >= LONG_PRESS_MS) {
                    // 长按：强制进入配网模式
                    current_mode = MODE_NET_CFG;
                    ESP_LOGI(TAG, "Long press detected, forced to NET_CFG mode");
                    
                    if (mode_change_callback) {
                        mode_change_callback(current_mode);
                    }
                    key_state = KEY_STATE_LONG_PRESS;
                }
                break;
                
            case KEY_STATE_LONG_PRESS:
                // 等待长按释放
                if (current_level) {
                    key_state = KEY_STATE_IDLE;
                    ESP_LOGD(TAG, "Key released after long press");
                }
                break;
        }
        
        last_key_level = current_level;
        vTaskDelay(pdMS_TO_TICKS(10));  // 10ms轮询间隔
    }
}

sys_mode_t key_get_current_mode(void)
{
    return current_mode;
}