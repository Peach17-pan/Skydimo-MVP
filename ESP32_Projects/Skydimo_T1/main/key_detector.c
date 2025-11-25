#include "key_detector.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "key_detector";

static key_event_callback_t short_press_callback = NULL;
static key_event_callback_t long_press_callback = NULL;
static TimerHandle_t long_press_timer = NULL;
static bool key_pressed = false;

// 长按定时器回调
static void long_press_timer_callback(TimerHandle_t xTimer)
{
    if (key_pressed && long_press_callback) {
        ESP_LOGI(TAG, "长按触发");
        long_press_callback();
    }
}

// GPIO中断处理函数
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    int level = gpio_get_level(EXAMPLE_PIN_NUM_KEY);
    
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    if (level == 0) { // 按键按下
        ESP_LOGI(TAG, "按键按下");
        if (!key_pressed) {
            key_pressed = true;
            // 启动长按定时器
            xTimerStartFromISR(long_press_timer, &xHigherPriorityTaskWoken);
        }
    } else { // 按键释放
        ESP_LOGI(TAG, "按键释放");
        if (key_pressed) {
            key_pressed = false;
            // 停止长按定时器
            xTimerStopFromISR(long_press_timer, &xHigherPriorityTaskWoken);
            
            // 如果是短按，触发短按回调
            if (short_press_callback) {
                ESP_LOGI(TAG, "短按触发");
                short_press_callback();
            }
        }
    }
    
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

void key_detector_init(key_event_callback_t short_press_cb, 
                      key_event_callback_t long_press_cb)
{
    short_press_callback = short_press_cb;
    long_press_callback = long_press_cb;
    
    ESP_LOGI(TAG, "初始化按键检测，GPIO: %d", EXAMPLE_PIN_NUM_KEY);
    
    // 配置GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << EXAMPLE_PIN_NUM_KEY),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,  // 启用上拉
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,    // 双边沿触发
    };
    
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GPIO配置失败: %s", esp_err_to_name(ret));
        return;
    }
    
    // 创建长按定时器
    long_press_timer = xTimerCreate("long_press", 
                                   pdMS_TO_TICKS(KEY_LONG_PRESS_TIME_MS),
                                   pdFALSE, NULL, long_press_timer_callback);
    
    if (long_press_timer == NULL) {
        ESP_LOGE(TAG, "长按定时器创建失败");
        return;
    }
    
    // 安装GPIO中断服务
    ret = gpio_install_isr_service(0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "GPIO中断服务安装失败: %s", esp_err_to_name(ret));
        return;
    }
    
    // 添加中断处理函数
    ret = gpio_isr_handler_add(EXAMPLE_PIN_NUM_KEY, gpio_isr_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GPIO中断处理函数添加失败: %s", esp_err_to_name(ret));
        return;
    }
    
    // 测试读取按键状态
    int key_state = gpio_get_level(EXAMPLE_PIN_NUM_KEY);
    ESP_LOGI(TAG, "按键初始状态: %s", key_state ? "高电平" : "低电平");
    
    ESP_LOGI(TAG, "按键检测器初始化完成");
}

bool is_key_pressed(void)
{
    return key_pressed;
}