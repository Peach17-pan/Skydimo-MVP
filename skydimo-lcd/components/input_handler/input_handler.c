#include "input_handler.h"

#include "esp_log.h"
#include "freertos/task.h"

static const char *TAG = "input_handler";
static system_handle_t *system_handle = NULL;
static bool button_pressed = false;
static uint32_t button_press_time = 0;

esp_err_t input_handler_init(system_handle_t *sys_handle) {
    ESP_LOGI(TAG, "初始化输入处理器");
    system_handle = sys_handle;
    
    // 配置BOOT按钮
    gpio_config_t btn_config = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&btn_config);
    
    // 配置LED指示灯
    gpio_config_t led_config = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_config);
    
    // 安装GPIO中断服务
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, input_button_isr_handler, NULL);
    
    // 创建LED闪烁任务
    xTaskCreate(input_led_task, "led_task", 2048, NULL, 2, NULL);
    
    ESP_LOGI(TAG, "输入处理器初始化完成");
    return ESP_OK;
}

void IRAM_ATTR input_button_isr_handler(void *arg) {
    int level = gpio_get_level(BUTTON_PIN);
    uint32_t current_time = xTaskGetTickCountFromISR();
    
    if (level == 0) { // 按钮按下
        button_press_time = current_time;
        button_pressed = true;
    } else { // 按钮释放
        if (button_pressed) {
            button_pressed = false;
            uint32_t press_duration = current_time - button_press_time;
            
            input_event_t event;
            if (press_duration >= pdMS_TO_TICKS(LONG_PRESS_DURATION)) {
                event.type = EVENT_BUTTON_LONG_PRESS;
                event.duration = press_duration;
                ESP_LOGI(TAG, "检测到长按，持续时间: %lums", press_duration);
            } else {
                event.type = EVENT_BUTTON_SHORT_PRESS;
                ESP_LOGI(TAG, "检测到短按");
            }
            
            // 发送事件到主任务
            if (system_handle && system_handle->event_queue) {
                BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                xQueueSendFromISR(system_handle->event_queue, &event, &xHigherPriorityTaskWoken);
                if (xHigherPriorityTaskWoken) {
                    portYIELD_FROM_ISR();
                }
            }
        }
    }
}

void input_led_task(void *pvParameters) {
    uint32_t counter = 0;
    
    while (1) {
        // LED闪烁指示系统运行状态
        gpio_set_level(LED_PIN, counter % 2);
        
        // 根据当前模式调整闪烁频率
        uint32_t delay_ms = 500; // 默认500ms
        if (system_handle) {
            switch (system_handle->current_mode) {
                case MODE_WIFI:
                    delay_ms = 200; // 快速闪烁
                    break;
                case MODE_CLOCK:
                    delay_ms = 1000; // 慢速闪烁
                    break;
                default:
                    delay_ms = 500;
                    break;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
        counter++;
    }
}