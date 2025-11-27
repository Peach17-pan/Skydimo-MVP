// main.c (主要修改部分)
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "input_handler.h"
#include "ui_manager.h"

static const char *TAG = "main";

// 全局系统句柄
static system_handle_t system_handle = {
    .panel_handle = NULL,
    .display_initialized = false,
    .current_mode = MODE_WIFI,
    .mode_start_time = 0,
    .event_queue = NULL,
    .lv_display = NULL,
    .touch_indev = NULL
};

// 事件处理任务
void event_handler_task(void *pvParameters) {
    input_event_t event;
    
    while (1) {
        if (xQueueReceive(system_handle.event_queue, &event, portMAX_DELAY)) {
            switch (event.type) {
                case EVENT_BUTTON_SHORT_PRESS:
                    // 短按切换模式
                    system_handle.current_mode = (system_handle.current_mode + 1) % MODE_MAX;
                    ESP_LOGI(TAG, "短按切换模式: %s", ui_get_mode_name(system_handle.current_mode));
                    
                    // 更新UI显示
                    if (system_handle.display_initialized) {
                        ui_show_current_mode(&system_handle);
                    }
                    break;
                    
                case EVENT_BUTTON_LONG_PRESS:
                    ESP_LOGI(TAG, "长按事件，持续时间: %lu", event.duration);
                    // 长按进入配网模式
                    if (system_handle.display_initialized) {
                        ui_handle_long_press(&system_handle);
                    }
                    break;
                    
                case EVENT_TOUCH:
                    if (system_handle.display_initialized) {
                        ui_handle_touch(&system_handle, event.x, event.y);
                    }
                    break;
                    
                default:
                    ESP_LOGW(TAG, "未知事件类型: %d", event.type);
                    break;
            }
        }
    }
}

// 初始化系统
void initialize_system(void) {
    ESP_LOGI(TAG, "初始化系统...");
    
    // 初始化NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGI(TAG, "擦除NVS并重新初始化");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // 创建事件队列
    system_handle.event_queue = xQueueCreate(10, sizeof(input_event_t));
    if (system_handle.event_queue == NULL) {
        ESP_LOGE(TAG, "创建事件队列失败");
        return;
    }
    
    // 初始化UI管理器
    ret = ui_manager_init(&system_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UI管理器初始化失败: %s", esp_err_to_name(ret));
        system_handle.display_initialized = false;
    } else {
        ESP_LOGI(TAG, "UI管理器初始化成功");
        system_handle.display_initialized = true;
    }
    
    // 初始化输入处理器
    ret = input_handler_init(&system_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "输入处理器初始化失败: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "输入处理器初始化成功");
        // 创建按钮检测任务
        xTaskCreate(input_check_button_task, "button_check", 4096, NULL, 1, NULL);
        // 创建LED闪烁任务
        xTaskCreate(input_led_task, "led_task", 2048, NULL, 1, NULL);
    }
    
    // 创建事件处理任务
    xTaskCreate(event_handler_task, "event_handler", 4096, NULL, 2, NULL);
    
    ESP_LOGI(TAG, "系统初始化完成");
    ESP_LOGI(TAG, "当前模式: %s", ui_get_mode_name(system_handle.current_mode));
}

// 状态监控任务
void status_task(void *pvParameters) {
    system_mode_t last_mode = system_handle.current_mode;
    
    // 显示初始模式
    if (system_handle.display_initialized) {
        ui_show_current_mode(&system_handle);
    }
    
    while (1) {
        // 检查模式变化
        if (system_handle.current_mode != last_mode) {
            ESP_LOGI(TAG, "=== 模式切换: %s -> %s ===", 
                    ui_get_mode_name(last_mode), 
                    ui_get_mode_name(system_handle.current_mode));
            
            if (system_handle.display_initialized) {
                ui_show_current_mode(&system_handle);
            }
            last_mode = system_handle.current_mode;
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "=== Skydimo MVP 启动 ===");
    
    // 给硬件上电稳定时间
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    ESP_LOGI(TAG, "开始系统初始化...");
    initialize_system();
    
    // 创建状态监控任务
    xTaskCreate(status_task, "status_task", 4096, NULL, 1, NULL);
    
    ESP_LOGI(TAG, "系统启动完成");
    ESP_LOGI(TAG, "使用BOOT按钮切换模式：短按切换，长按进入配网");
    
    // 主循环
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
    ESP_LOGI(TAG, "初始化UI管理器");
    
    // 创建LVGL互斥锁
    lvgl_mutex = xSemaphoreCreateMutex();
    if (lvgl_mutex == NULL) {
        ESP_LOGE(TAG, "创建LVGL互斥锁失败");
        return ESP_FAIL;
    }
    
    // 分配显示缓冲区
    size_t buffer_size = EXAMPLE_LCD_H_RES * 40; // 40行缓冲区
    uint8_t *buf1 = heap_caps_malloc(buffer_size, MALLOC_CAP_DMA);
    uint8_t *buf2 = heap_caps_malloc(buffer_size, MALLOC_CAP_DMA);