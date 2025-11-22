#include "mode_manager.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

static const char *TAG = "ModeManager";

static device_mode_t current_mode = MODE_CLOCK; // 默认时钟模式
static QueueHandle_t mode_change_queue = NULL;

// 模式名称定义
const char* MODE_NAMES[MODE_COUNT] = {
    "配网模式",
    "时钟模式", 
    "天气模式",
    "相册模式",
    "虚拟键盘模式"
};

void mode_manager_init(void) {
    // 创建模式切换事件队列
    mode_change_queue = xQueueCreate(5, sizeof(mode_change_event_t));
    if (mode_change_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create mode change queue");
    }
    
    ESP_LOGI(TAG, "Mode manager initialized, default mode: %s", MODE_NAMES[current_mode]);
}

device_mode_t get_current_mode(void) {
    return current_mode;
}

void switch_to_next_mode(void) {
    device_mode_t old_mode = current_mode;
    
    // 循环切换到下一个模式
    current_mode = (device_mode_t)((current_mode + 1) % MODE_COUNT);
    
    ESP_LOGI(TAG, "Mode changed: %s -> %s", 
             MODE_NAMES[old_mode], MODE_NAMES[current_mode]);
    
    // 发送模式切换事件
    if (mode_change_queue != NULL) {
        mode_change_event_t event = {
            .old_mode = old_mode,
            .new_mode = current_mode
        };
        
        xQueueSend(mode_change_queue, &event, portMAX_DELAY);
    }
}

void switch_to_mode(device_mode_t mode) {
    if (mode >= MODE_COUNT) return;
    
    device_mode_t old_mode = current_mode;
    current_mode = mode;
    
    ESP_LOGI(TAG, "Mode changed: %s -> %s", 
             MODE_NAMES[old_mode], MODE_NAMES[current_mode]);
    
    // 发送模式切换事件
    if (mode_change_queue != NULL) {
        mode_change_event_t event = {
            .old_mode = old_mode,
            .new_mode = current_mode
        };
        
        xQueueSend(mode_change_queue, &event, portMAX_DELAY);
    }
}

QueueHandle_t get_mode_change_queue(void) {
    return mode_change_queue;
}