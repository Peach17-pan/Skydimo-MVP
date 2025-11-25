#include "mode_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

static const char *TAG = "mode_manager";

static device_mode_t current_mode = MODE_CLOCK;  // 默认时钟模式
static SemaphoreHandle_t mode_mutex = NULL;

// 模式名称数组
static const char* mode_names[MODE_MAX] = {
    "配网模式",
    "时钟模式", 
    "天气模式",
    "相册模式",
    "虚拟键盘模式"
};

void mode_manager_init(void)
{
    // 创建互斥锁保护模式切换
    mode_mutex = xSemaphoreCreateMutex();
    if (mode_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mode mutex");
    }
    ESP_LOGI(TAG, "Mode manager initialized, default mode: %s", mode_names[current_mode]);
}

device_mode_t get_current_mode(void)
{
    device_mode_t mode = current_mode;  // 初始化为当前模式，确保有默认值
    
    if (mode_mutex && xSemaphoreTake(mode_mutex, portMAX_DELAY)) {
        mode = current_mode;
        xSemaphoreGive(mode_mutex);
    }
    return mode;
}

void switch_to_next_mode(void)
{
    if (mode_mutex && xSemaphoreTake(mode_mutex, portMAX_DELAY)) {
        device_mode_t old_mode = current_mode;
        current_mode = (device_mode_t)((current_mode + 1) % MODE_MAX);
        xSemaphoreGive(mode_mutex);
        
        ESP_LOGI(TAG, "Mode switched from %s to %s", 
                mode_names[old_mode], mode_names[current_mode]);
    }
}

void switch_to_mode(device_mode_t new_mode)
{
    if (new_mode >= MODE_MAX) {
        ESP_LOGW(TAG, "Invalid mode: %d", new_mode);
        return;
    }
    
    if (mode_mutex && xSemaphoreTake(mode_mutex, portMAX_DELAY)) {
        device_mode_t old_mode = current_mode;
        current_mode = new_mode;
        xSemaphoreGive(mode_mutex);
        
        if (old_mode != new_mode) {
            ESP_LOGI(TAG, "Mode switched from %s to %s", 
                    mode_names[old_mode], mode_names[new_mode]);
        }
    }
}

const char* get_mode_name(device_mode_t mode)
{
    if (mode < MODE_MAX) {
        return mode_names[mode];
    }
    return "未知模式";
}