#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "button.h"
#include "mode_manager.h"
#include "ui_port.h"
#include "lcd_touch.h"

static const char *TAG = "MAIN";

void button_task(void *pvParam)
{
    button_init();
    
    // 初始显示当前模式
    mode_t current_mode = mode_get();
    ui_switch(current_mode);
    ESP_LOGI(TAG, "系统启动，初始模式: %d", current_mode);

    while (1) {
        int action = button_scan();
        
        switch (action) {
            case 1:  // 短按 - 切换模式
                current_mode = mode_next();
                ui_switch(current_mode);
                ESP_LOGI(TAG, "短按 - 切换到模式: %d", current_mode);
                break;
                
            case 2:  // 长按 - 进入配网模式（为任务2预留）
                ESP_LOGI(TAG, "长按 - 进入配网模式");
                // 这里可以强制切换到配网模式，或者添加配网逻辑
                break;
                
            default:
                break;
        }
        
        vTaskDelay(pdMS_TO_TICKS(10)); // 减少延迟，提高响应速度
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== 多功能显示系统启动 ===");
    ESP_LOGI(TAG, "硬件初始化...");
    
    // 初始化LCD触摸
    lcd_touch_init();
    
    // 创建按键任务
    xTaskCreate(button_task, "button_task", 4096, NULL, 10, NULL);
    
    ESP_LOGI(TAG, "系统初始化完成，等待用户操作...");
    
    // 主循环保持空闲
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}