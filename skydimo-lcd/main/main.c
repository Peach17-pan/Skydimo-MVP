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
    .event_queue = NULL
};

// 获取模式名称
const char* get_mode_name(int mode) {
    static const char* mode_names[] = {
        "WiFi配网", "时钟", "天气", "相册", "虚拟键盘"
    };
    return (mode < MODE_MAX) ? mode_names[mode] : "未知";
}

// 事件处理任务
void event_handler_task(void *pvParameters) {
    input_event_t event;
    
    while (1) {
        if (xQueueReceive(system_handle.event_queue, &event, portMAX_DELAY)) {
            switch (event.type) {
                case EVENT_BUTTON_SHORT_PRESS:
                    // 短按切换模式
                    system_handle.current_mode = (system_handle.current_mode + 1) % MODE_MAX;
                    ESP_LOGI(TAG, "短按切换模式: %s", get_mode_name(system_handle.current_mode));
                    
                    // 更新UI显示
                    if (system_handle.display_initialized) {
                        ui_show_current_mode(&system_handle);
                    }
                    break;
                    
                case EVENT_BUTTON_LONG_PRESS:
                    ESP_LOGI(TAG, "长按事件，持续时间: %lu", event.duration);
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
        ESP_LOGE(TAG, "UI管理器初始化失败");
    } else {
        ESP_LOGI(TAG, "UI管理器初始化成功");
    }
    
    // 初始化输入处理器
    ret = input_handler_init(&system_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "输入处理器初始化失败");
    } else {
        ESP_LOGI(TAG, "输入处理器初始化成功");
    }
    
    // 创建事件处理任务
    xTaskCreate(event_handler_task, "event_handler", 4096, NULL, 3, NULL);
    
    ESP_LOGI(TAG, "系统初始化完成");
    ESP_LOGI(TAG, "当前模式: %s", get_mode_name(system_handle.current_mode));
}

// 状态监控任务
void status_task(void *pvParameters) {
    uint32_t counter = 0;
    int last_mode = system_handle.current_mode;
    
    // 显示初始模式
    if (system_handle.display_initialized) {
        ui_show_current_mode(&system_handle);
    }
    
    while (1) {
        // 检查模式变化
        if (system_handle.current_mode != last_mode) {
            ESP_LOGI(TAG, "=== 模式已切换 ===");
            if (system_handle.display_initialized) {
                ui_show_current_mode(&system_handle);
            }
            last_mode = system_handle.current_mode;
        }
        
        // 定期更新状态显示
        if (system_handle.display_initialized && (counter % 20 == 0)) {
            ui_update_status_display(&system_handle);
        }
        
        // 每5秒打印一次状态
        if (counter % 10 == 0) {
            ESP_LOGI(TAG, "系统运行中... 计数: %lu, 显示: %s", 
                    counter/2, system_handle.display_initialized ? "就绪" : "未就绪");
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));
        counter++;
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "=== Skydimo MVP 启动 ===");
    
    initialize_system();
    
    // 创建状态监控任务
    xTaskCreate(status_task, "status_task", 4096, NULL, 2, NULL);
    
    ESP_LOGI(TAG, "系统启动完成，开始运行...");
    
    // 主循环
    while (1) {
        // 主循环保持运行
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}