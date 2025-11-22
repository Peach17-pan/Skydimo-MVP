#include "button.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define GPIO_BTN     0
#define DEBOUNCE_MS  20
#define LONG_PRESS_MS 1000

void button_init(void)
{
    gpio_config_t io = {
        .pin_bit_mask = BIT64(GPIO_BTN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);
}

int button_scan(void)
{
    if (gpio_get_level(GPIO_BTN) == 0) {
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
        if (gpio_get_level(GPIO_BTN) == 0) {
            uint32_t press_time = xTaskGetTickCount();
            while (gpio_get_level(GPIO_BTN) == 0) {
                if ((xTaskGetTickCount() - press_time) * portTICK_PERIOD_MS > LONG_PRESS_MS) {
                    while (gpio_get_level(GPIO_BTN) == 0) vTaskDelay(pdMS_TO_TICKS(10));
                    return 2; // 长按
                }
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            return 1; // 短按
        }
    }
    return 0;
}