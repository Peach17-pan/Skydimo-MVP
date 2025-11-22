#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"

#include "user_config.h"
#include "button_bsp.h"
#include "mode_manager.h"
#include "lcd_bl_pwm_bsp.h"
#include "i2c_bsp.h"
#include "lcd_driver.h"

static const char *TAG = "MAIN";

// 系统状态监控任务
static void system_monitor_task(void *arg)
{
    int counter = 0;
    while (1) {
        ESP_LOGI("MONITOR", "System running - Counter: %d, Heap: %lu bytes", 
                counter++, (unsigned long)esp_get_free_heap_size());
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// 硬件初始化函数
static void hardware_init(void)
{
    ESP_LOGI(TAG, "Starting hardware initialization...");
    
    // 初始化背光
    lcd_bl_pwm_bsp_init(LCD_PWM_MODE_100);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // 初始化LCD驱动
    lcd_driver_init();
    
    // 添加屏幕测试
    ESP_LOGI(TAG, "Testing LCD display...");
    lcd_test_pattern();  // 显示测试图案
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    // 初始化其他硬件
    // touch_i2c_master_Init();  // 暂时注释触摸
    button_init();
    
    ESP_LOGI(TAG, "Hardware initialization completed");
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== LCD Diagnostic Test ===");
    
    // 初始化背光
    lcd_bl_pwm_bsp_init(LCD_PWM_MODE_100);
    vTaskDelay(pdMS_TO_TICKS(200));
    
    // 初始化LCD驱动
    lcd_driver_init();
    
    // 运行详细测试
    lcd_detailed_test();
    
    ESP_LOGI(TAG, "=== All Tests Completed ===");
    
    // 保持系统运行
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        ESP_LOGI(TAG, "System idle - Heap: %lu", (unsigned long)esp_get_free_heap_size());
    }
}

static void mode_display_task(void *arg)
{
    while(1) {
        // 这里应该从模式管理器获取当前模式并显示
        ESP_LOGI("MODE", "Current mode placeholder");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}