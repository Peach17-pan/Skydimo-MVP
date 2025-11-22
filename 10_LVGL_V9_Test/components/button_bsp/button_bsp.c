#include "button_bsp.h"
#include "../../main/user_config.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "BUTTON";

// 按键事件队列
static QueueHandle_t button_event_queue = NULL;

// 按键状态
static volatile uint64_t button_press_time = 0;
static volatile bool button_pressed = false;

#define BUTTON_DEBOUNCE_MS    50
#define BUTTON_LONG_PRESS_MS  2000

static void IRAM_ATTR button_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    int level = gpio_get_level(gpio_num);
    
    // 只在按键释放时处理，避免重复触发
    if (level == 1 && button_pressed) {
        button_pressed = false;
        uint64_t current_time = esp_timer_get_time() / 1000;
        uint64_t press_duration = current_time - button_press_time;
        
        if (press_duration > BUTTON_DEBOUNCE_MS) {
            button_event_t event;
            
            if (press_duration >= BUTTON_LONG_PRESS_MS) {
                event = BUTTON_LONG_PRESS;
                ESP_LOGI(TAG, "长按事件: %llu ms", press_duration);
            } else {
                event = BUTTON_SHORT_PRESS;
                ESP_LOGI(TAG, "短按事件: %llu ms", press_duration);
            }
            
            // 发送事件
            if (button_event_queue != NULL) {
                xQueueSendFromISR(button_event_queue, &event, NULL);
            }
        }
    } else if (level == 0 && !button_pressed) {
        // 按键按下，记录时间
        button_pressed = true;
        button_press_time = esp_timer_get_time() / 1000;
    }
}

void button_init(void)
{
    ESP_LOGI(TAG, "初始化按键，GPIO: %d", EXAMPLE_PIN_NUM_KEY);
    
    // 创建按键事件队列
    button_event_queue = xQueueCreate(10, sizeof(button_event_t));
    if (button_event_queue == NULL) {
        ESP_LOGE(TAG, "创建按键队列失败");
        return;
    }
    
    // 配置GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << EXAMPLE_PIN_NUM_KEY),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };
    
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GPIO配置失败: %s", esp_err_to_name(ret));
        return;
    }
    
    // 安装GPIO ISR服务
    ret = gpio_install_isr_service(0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "安装ISR服务失败: %s", esp_err_to_name(ret));
        return;
    }
    
    // 添加ISR处理程序
    ret = gpio_isr_handler_add(EXAMPLE_PIN_NUM_KEY, button_isr_handler, (void*) EXAMPLE_PIN_NUM_KEY);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "添加ISR处理程序失败: %s", esp_err_to_name(ret));
        return;
    }
    
    ESP_LOGI(TAG, "按键初始化完成");
}

QueueHandle_t get_button_event_queue(void)
{
    return button_event_queue;
}