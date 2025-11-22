#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lvgl.h"
#include "user_config.h"
#include "key_driver.h"
#include "ui_mode.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"

static const char *TAG = "MAIN";

// LVGL任务句柄
static TaskHandle_t lvgl_task_handle = NULL;

/**
 * @brief 模式切换回调函数
 */
static void on_mode_change(sys_mode_t new_mode)
{
    ESP_LOGI(TAG, "============================");
    ESP_LOGI(TAG, "MODE CHANGE: %d", new_mode);
    ESP_LOGI(TAG, "============================");
    
    // 切换UI
    ui_switch_mode(new_mode);
}

/**
 * @brief LVGL主任务
 */
static void lvgl_task(void *arg)
{
    ESP_LOGI(TAG, "LVGL task started");
    
    while (1) {
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(5));  // LVGL推荐5ms轮询
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "===================================");
    ESP_LOGI(TAG, "Multi-Screen System Starting...");
    ESP_LOGI(TAG, "===================================");
    
    // 1. 初始化LVGL核心
    lv_init();
    ESP_LOGI(TAG, "LVGL initialized");
    
    // 2. 初始化显示端口（SPI LCD）
    lv_port_disp_init();
    ESP_LOGI(TAG, "Display port initialized");
    
    // 3. 初始化输入设备端口（触摸屏，任务1可暂不实现）
    // lv_port_indev_init();
    // ESP_LOGI(TAG, "Input device port initialized");
    
    // 4. 初始化UI模块
    ui_init();
    ESP_LOGI(TAG, "UI module initialized");
    
    // 5. 初始化按键驱动
    key_driver_init(on_mode_change);
    ESP_LOGI(TAG, "Key driver initialized");
    
    // 6. 创建LVGL任务
    xTaskCreate(lvgl_task, "lvgl_task", 4096, NULL, 5, &lvgl_task_handle);
    
    // 7. 创建按键任务
    xTaskCreate(key_driver_task, "key_task", 2048, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "===================================");
    ESP_LOGI(TAG, "System started successfully!");
    ESP_LOGI(TAG, "Press KEY0 to switch modes");
    ESP_LOGI(TAG, "Long press KEY0 to force NET_CFG");
    ESP_LOGI(TAG, "===================================");
    
    // 主循环
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}