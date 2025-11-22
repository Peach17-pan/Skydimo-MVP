#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "button.h"
#include "mode_manager.h"
#include "ui_port.h"
#include "lcd_touch.h"

#define TAG "MAIN"

void button_task(void *pvParam)
{
    button_init();
    ui_switch(mode_get());

    while (1) {
        int action = button_scan();
        if (action == 1) {
            mode_t current_mode = mode_next();
            ui_switch(current_mode);
            ESP_LOGI(TAG, "切换到模式: %d", current_mode);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void app_main(void)
{
    lcd_touch_init();
    xTaskCreate(button_task, "button_task", 4096, NULL, 10, NULL);
    
    while (1) vTaskDelay(pdMS_TO_TICKS(1000));
}