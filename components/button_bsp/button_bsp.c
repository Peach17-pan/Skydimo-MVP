/**
 * @file button_bsp.c
 * @brief 按键 BSP 实现
 */

#include "button_bsp.h"
#include "multi_button.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"

static const char *TAG = "button_bsp";

/* 事件组句柄 */
EventGroupHandle_t boot_key_event;
EventGroupHandle_t pwr_key_event;

/* 按键对象 */
static Button boot_button;
static Button pwr_button;

/* 按键 ID 定义 */
#define BOOT_BUTTON_ID      1
#define PWR_BUTTON_ID       2
#define BUTTON_ACTIVE_LEVEL 0   /* 低电平有效 */

/******************** 回调函数声明 ********************/
static void on_boot_single_click(Button* btn);
static void on_boot_double_click(Button* btn);
static void on_boot_long_press(Button* btn);
static void on_boot_press_up(Button* btn);

static void on_pwr_single_click(Button* btn);
static void on_pwr_double_click(Button* btn);
static void on_pwr_long_press(Button* btn);
static void on_pwr_press_up(Button* btn);

/******************** 定时器回调 ********************/
static void button_tick_callback(void *arg)
{
    button_ticks();
}

/******************** GPIO 读取回调 ********************/
static uint8_t read_button_gpio(uint8_t button_id)
{
    switch (button_id) {
        case BOOT_BUTTON_ID:
            return gpio_get_level(USER_KEY_BOOT);
        case PWR_BUTTON_ID:
            return gpio_get_level(USER_KEY_PWR);
        default:
            return 1;
    }
}

/******************** GPIO 初始化 ********************/
static void button_gpio_init(void)
{
    gpio_config_t gpio_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = ((uint64_t)1 << USER_KEY_BOOT) | ((uint64_t)1 << USER_KEY_PWR),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));
}

/******************** 公共函数实现 ********************/
void button_bsp_init(void)
{
    ESP_LOGI(TAG, "Initializing buttons...");
    
    /* 创建事件组 */
    boot_key_event = xEventGroupCreate();
    pwr_key_event = xEventGroupCreate();
    
    /* 初始化 GPIO */
    button_gpio_init();
    
    /* 初始化 BOOT 按键 */
    button_init(&boot_button, read_button_gpio, BUTTON_ACTIVE_LEVEL, BOOT_BUTTON_ID);
    button_attach(&boot_button, BTN_SINGLE_CLICK, on_boot_single_click);
    button_attach(&boot_button, BTN_PRESS_REPEAT, on_boot_double_click);
    button_attach(&boot_button, BTN_LONG_PRESS_START, on_boot_long_press);
    button_attach(&boot_button, BTN_PRESS_UP, on_boot_press_up);
    
    /* 初始化 PWR 按键 */
    button_init(&pwr_button, read_button_gpio, BUTTON_ACTIVE_LEVEL, PWR_BUTTON_ID);
    button_attach(&pwr_button, BTN_SINGLE_CLICK, on_pwr_single_click);
    button_attach(&pwr_button, BTN_DOUBLE_CLICK, on_pwr_double_click);
    button_attach(&pwr_button, BTN_LONG_PRESS_START, on_pwr_long_press);
    button_attach(&pwr_button, BTN_PRESS_UP, on_pwr_press_up);
    
    /* 创建定时器 */
    const esp_timer_create_args_t timer_args = {
        .callback = &button_tick_callback,
        .name = "button_tick",
        .arg = NULL,
    };
    esp_timer_handle_t timer_handle = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timer_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer_handle, 5 * 1000));  /* 5ms */
    
    /* 启动按键 */
    button_start(&boot_button);
    button_start(&pwr_button);
    
    ESP_LOGI(TAG, "Buttons initialized successfully");
}

uint8_t button_pwr_get_repeat_count(void)
{
    return button_get_repeat_count(&pwr_button);
}

uint8_t button_boot_get_repeat_count(void)
{
    return button_get_repeat_count(&boot_button);
}

/******************** BOOT 按键回调实现 ********************/
static void on_boot_single_click(Button* btn)
{
    ESP_LOGI(TAG, "BOOT: Single click");
    xEventGroupSetBits(boot_key_event, KEY_EVENT_SINGLE_CLICK);
}

static void on_boot_double_click(Button* btn)
{
    ESP_LOGI(TAG, "BOOT: Double click");
    xEventGroupSetBits(boot_key_event, KEY_EVENT_DOUBLE_CLICK);
}

static void on_boot_long_press(Button* btn)
{
    ESP_LOGI(TAG, "BOOT: Long press");
    xEventGroupSetBits(boot_key_event, KEY_EVENT_LONG_PRESS);
}

static void on_boot_press_up(Button* btn)
{
    xEventGroupSetBits(boot_key_event, KEY_EVENT_PRESS_UP);
}

/******************** PWR 按键回调实现 ********************/
static void on_pwr_single_click(Button* btn)
{
    ESP_LOGI(TAG, "PWR: Single click");
    xEventGroupSetBits(pwr_key_event, KEY_EVENT_SINGLE_CLICK);
}

static void on_pwr_double_click(Button* btn)
{
    ESP_LOGI(TAG, "PWR: Double click");
    xEventGroupSetBits(pwr_key_event, KEY_EVENT_DOUBLE_CLICK);
}

static void on_pwr_long_press(Button* btn)
{
    ESP_LOGI(TAG, "PWR: Long press");
    xEventGroupSetBits(pwr_key_event, KEY_EVENT_LONG_PRESS);
}

static void on_pwr_press_up(Button* btn)
{
    xEventGroupSetBits(pwr_key_event, KEY_EVENT_PRESS_UP);
}

