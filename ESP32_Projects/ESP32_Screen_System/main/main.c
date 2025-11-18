#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/spi_master.h"
// 移除 esp_timer.h，因为我们不需要它
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "driver/gpio.h"
#include "esp_task_wdt.h"

// 显示引脚配置
#define LCD_HOST SPI3_HOST
#define EXAMPLE_PIN_NUM_LCD_CS     GPIO_NUM_9
#define EXAMPLE_PIN_NUM_LCD_PCLK   GPIO_NUM_10
#define EXAMPLE_PIN_NUM_LCD_DATA0  GPIO_NUM_11
#define EXAMPLE_PIN_NUM_LCD_DATA1  GPIO_NUM_12
#define EXAMPLE_PIN_NUM_LCD_DATA2  GPIO_NUM_13
#define EXAMPLE_PIN_NUM_LCD_DATA3  GPIO_NUM_14
#define EXAMPLE_PIN_NUM_LCD_RST    GPIO_NUM_21
#define EXAMPLE_PIN_NUM_BK_LIGHT   GPIO_NUM_8

#define EXAMPLE_LCD_H_RES 172
#define EXAMPLE_LCD_V_RES 640

// 按键配置
#define BUTTON_PIN GPIO_NUM_6
#define BUTTON_DEBOUNCE_MS 50

static const char *TAG = "SCREEN_SYSTEM";

// LCD面板句柄
static esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_panel_io_handle_t io_handle = NULL;

// 系统模式定义
typedef enum {
    MODE_NETWORK = 0,
    MODE_CLOCK,
    MODE_WEATHER,
    MODE_GALLERY,
    MODE_VIRTUAL_KEYBOARD,
    MODE_COUNT
} system_mode_t;

// 全局变量
static volatile system_mode_t current_mode = MODE_CLOCK;
static QueueHandle_t button_queue = NULL;
static bool lcd_initialized = false;

/**
 * @brief 初始化背光控制
 */
void init_backlight(void)
{
    ESP_LOGI(TAG, "初始化背光控制...");
    
    gpio_config_t bk_gpio_config = {
        .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_BK_LIGHT,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    
    gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, 0);
    ESP_LOGI(TAG, "背光控制初始化完成");
}

/**
 * @brief 开启背光
 */
void enable_backlight(void)
{
    gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, 1);
    ESP_LOGI(TAG, "背光已开启");
}

/**
 * @brief 初始化按键
 */
void init_button(void)
{
    gpio_config_t btn_config = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&btn_config));
    
    button_queue = xQueueCreate(10, sizeof(uint32_t));
    ESP_LOGI(TAG, "按键初始化完成");
}

/**
 * @brief 按键中断处理函数
 */
static void IRAM_ATTR button_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(button_queue, &gpio_num, NULL);
}

/**
 * @brief 硬件复位LCD
 */
void hardware_reset_lcd(void)
{
    ESP_LOGI(TAG, "执行LCD硬件复位...");
    
    gpio_config_t rst_config = {
        .pin_bit_mask = (1ULL << EXAMPLE_PIN_NUM_LCD_RST),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&rst_config);
    
    gpio_set_level(EXAMPLE_PIN_NUM_LCD_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(EXAMPLE_PIN_NUM_LCD_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(120));
    
    ESP_LOGI(TAG, "LCD硬件复位完成");
}

/**
 * @brief 初始化LCD显示 - 使用通用SPI面板
 */
esp_err_t init_lcd(void)
{
    esp_err_t ret;
    
    ESP_LOGI(TAG, "开始初始化LCD...");
    
    // 先执行硬件复位
    hardware_reset_lcd();
    
    // 初始化QSPI总线
    ESP_LOGI(TAG, "初始化QSPI总线...");
    spi_bus_config_t buscfg = {
        .data0_io_num = EXAMPLE_PIN_NUM_LCD_DATA0,
        .data1_io_num = EXAMPLE_PIN_NUM_LCD_DATA1,
        .data2_io_num = EXAMPLE_PIN_NUM_LCD_DATA2,
        .data3_io_num = EXAMPLE_PIN_NUM_LCD_DATA3,
        .sclk_io_num = EXAMPLE_PIN_NUM_LCD_PCLK,
        .max_transfer_sz = EXAMPLE_LCD_H_RES * 64 * 2,
    };
    
    ret = spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "QSPI总线初始化失败: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 配置LCD面板IO
    ESP_LOGI(TAG, "配置LCD面板IO...");
    esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = EXAMPLE_PIN_NUM_LCD_CS,
        .dc_gpio_num = -1,
        .spi_mode = 3,
        .pclk_hz = 10 * 1000 * 1000, // 降低频率
        .trans_queue_depth = 10,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .flags.quad_mode = true,
    };
    
    ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LCD IO初始化失败: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 使用通用面板配置
    ESP_LOGI(TAG, "安装通用面板驱动...");
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = -1,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    
    // 只尝试使用ST7789
    ret = esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ST7789面板驱动初始化失败: %s", esp_err_to_name(ret));
        spi_bus_free(LCD_HOST);
        return ret;
    }
    
    // 初始化面板
    ESP_LOGI(TAG, "初始化LCD面板...");
    ret = esp_lcd_panel_init(panel_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "面板初始化失败: %s", esp_err_to_name(ret));
        spi_bus_free(LCD_HOST);
        return ret;
    }
    
    // 开启显示
    esp_lcd_panel_disp_on_off(panel_handle, true);
    
    lcd_initialized = true;
    ESP_LOGI(TAG, "LCD初始化完成！");
    return ESP_OK;
}

/**
 * @brief 简单的填充屏幕函数
 */
void fill_screen(uint16_t color)
{
    if (!lcd_initialized) return;
    
    uint16_t *buffer = heap_caps_malloc(EXAMPLE_LCD_H_RES * 16 * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (!buffer) return;
    
    for (int i = 0; i < EXAMPLE_LCD_H_RES * 16; i++) {
        buffer[i] = color;
    }
    
    for (int y = 0; y < EXAMPLE_LCD_V_RES; y += 16) {
        int height = (y + 16 > EXAMPLE_LCD_V_RES) ? (EXAMPLE_LCD_V_RES - y) : 16;
        esp_lcd_panel_draw_bitmap(panel_handle, 0, y, EXAMPLE_LCD_H_RES, y + height, buffer);
    }
    
    free(buffer);
}

/**
 * @brief 显示测试图案
 */
void display_test_pattern(void)
{
    if (!lcd_initialized) return;
    
    ESP_LOGI(TAG, "显示测试图案...");
    
    fill_screen(0xF800); // 红
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    fill_screen(0x07E0); // 绿
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    fill_screen(0x001F); // 蓝
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    fill_screen(0x0000); // 黑
    vTaskDelay(pdMS_TO_TICKS(500));
}

/**
 * @brief 绘制模式界面
 */
void draw_mode_interface(system_mode_t mode)
{
    if (!lcd_initialized) return;
    
    uint16_t bg_color;
    switch (mode) {
        case MODE_NETWORK: bg_color = 0x001F; break; // 蓝
        case MODE_CLOCK: bg_color = 0x0000; break;   // 黑
        case MODE_WEATHER: bg_color = 0x07FF; break; // 青
        case MODE_GALLERY: bg_color = 0xF81F; break; // 洋红
        case MODE_VIRTUAL_KEYBOARD: bg_color = 0x07E0; break; // 绿
        default: bg_color = 0xF800; break; // 红
    }
    
    fill_screen(bg_color);
    ESP_LOGI(TAG, "切换到模式: %d", mode);
}

/**
 * @brief 按键任务
 */
void button_task(void *arg)
{
    uint32_t io_num;
    
    while (1) {
        if (xQueueReceive(button_queue, &io_num, portMAX_DELAY)) {
            vTaskDelay(pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS));
            if (gpio_get_level(io_num) == 0) {
                current_mode = (current_mode + 1) % MODE_COUNT;
                ESP_LOGI(TAG, "切换到模式: %d", current_mode);
                draw_mode_interface(current_mode);
            }
        }
    }
}

/**
 * @brief 应用程序入口
 */
void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32-S3 LCD系统启动 ===");
    
    // 初始化看门狗
    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = 10000,
        .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
        .trigger_panic = true,
    };
    esp_task_wdt_init(&twdt_config);
    esp_task_wdt_add(NULL);
    
    // 初始化硬件
    init_backlight();
    init_button();
    
    // 安装中断
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, (void *)BUTTON_PIN);
    
    // 初始化LCD
    if (init_lcd() == ESP_OK) {
        ESP_LOGI(TAG, "LCD初始化成功");
        enable_backlight();
        display_test_pattern();
        xTaskCreate(button_task, "button_task", 4096, NULL, 5, NULL);
        draw_mode_interface(current_mode);
        ESP_LOGI(TAG, "系统就绪，按按键切换模式");
    } else {
        ESP_LOGE(TAG, "LCD初始化失败");
        while(1) {
            gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, 1);
            vTaskDelay(pdMS_TO_TICKS(200));
            gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, 0);
            vTaskDelay(pdMS_TO_TICKS(200));
            esp_task_wdt_reset();
        }
    }
    
    while (1) {
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}