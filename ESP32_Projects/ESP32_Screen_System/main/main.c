#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"

// 显示配置 - 根据你的实际硬件修改这些引脚
#define LCD_PIN_NUM_MISO  -1
#define LCD_PIN_NUM_MOSI  11
#define LCD_PIN_NUM_CLK   12
#define LCD_PIN_NUM_CS    10
#define LCD_PIN_NUM_DC    9
#define LCD_PIN_NUM_RST   8
#define LCD_PIN_NUM_BL    7

#define LCD_H_RES         240
#define LCD_V_RES         320
#define LCD_BITS_PER_PIXEL 16

static const char *TAG = "SCREEN_SYSTEM";

// LCD面板句柄
static esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_panel_io_handle_t io_handle = NULL;

// 颜色定义 (RGB565)
#define COLOR_BLACK       0x0000
#define COLOR_WHITE       0xFFFF
#define COLOR_RED         0xF800
#define COLOR_GREEN       0x07E0
#define COLOR_BLUE        0x001F
#define COLOR_YELLOW      0xFFE0

/**
 * @brief 初始化背光控制
 */
void init_backlight(void)
{
    gpio_config_t bk_gpio_config = {
        .pin_bit_mask = 1ULL << LCD_PIN_NUM_BL,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    gpio_set_level(LCD_PIN_NUM_BL, 1); // 开启背光
    ESP_LOGI(TAG, "背光初始化完成，GPIO: %d", LCD_PIN_NUM_BL);
}

/**
 * @brief 绘制矩形区域（简化版清屏）
 */
void draw_color_screen(uint16_t color)
{
    // 创建一个单色缓冲区
    uint16_t *buffer = malloc(LCD_H_RES * sizeof(uint16_t));
    if (buffer == NULL) {
        ESP_LOGE(TAG, "内存分配失败");
        return;
    }
    
    // 填充颜色
    for (int x = 0; x < LCD_H_RES; x++) {
        buffer[x] = color;
    }
    
    // 逐行绘制
    for (int y = 0; y < LCD_V_RES; y++) {
        esp_lcd_panel_draw_bitmap(panel_handle, 0, y, LCD_H_RES, y + 1, buffer);
    }
    
    free(buffer);
}

/**
 * @brief 初始化LCD显示
 */
esp_err_t init_lcd(void)
{
    ESP_LOGI(TAG, "开始初始化LCD显示屏");
    
    // 初始化SPI总线
    spi_bus_config_t buscfg = {
        .miso_io_num = LCD_PIN_NUM_MISO,
        .mosi_io_num = LCD_PIN_NUM_MOSI,
        .sclk_io_num = LCD_PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * LCD_V_RES * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // 配置LCD面板IO
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = LCD_PIN_NUM_DC,
        .cs_gpio_num = LCD_PIN_NUM_CS,
        .pclk_hz = 20 * 1000 * 1000, // 降低到20MHz确保稳定性
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    
    // 修正：使用正确的句柄类型
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &io_handle));

    // 配置LCD面板 - 使用通用面板初始化
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_PIN_NUM_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = LCD_BITS_PER_PIXEL,
    };
    
    // 修正：使用正确的函数和句柄
    ESP_ERROR_CHECK(esp_lcd_new_panel_nt35510(io_handle, &panel_config, &panel_handle));

    // 重置显示屏
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));

    // 初始化面板
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    // 修正：使用新的API替代已弃用的函数
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI(TAG, "LCD显示屏初始化完成");
    return ESP_OK;
}

/**
 * @brief 显示测试图案
 */
void display_test_pattern(void)
{
    ESP_LOGI(TAG, "开始显示测试图案");
    
    // 红色
    draw_color_screen(COLOR_RED);
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // 绿色
    draw_color_screen(COLOR_GREEN);
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // 蓝色
    draw_color_screen(COLOR_BLUE);
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // 白色
    draw_color_screen(COLOR_WHITE);
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // 黑色
    draw_color_screen(COLOR_BLACK);
    vTaskDelay(pdMS_TO_TICKS(500));
    
    ESP_LOGI(TAG, "测试图案显示完成");
}

/**
 * @brief 在指定位置显示文本（简化版）
 */
void display_text(int x, int y, const char *text, uint16_t color)
{
    ESP_LOGI(TAG, "Display at (%d,%d): %s", x, y, text);
    // 这里先简单记录，实际需要实现文本渲染
}

/**
 * @brief 主显示任务
 */
void display_task(void *pvParameters)
{
    ESP_LOGI(TAG, "显示任务开始");
    
    // 显示初始界面
    draw_color_screen(COLOR_BLACK);
    
    // 模拟你的应用显示逻辑
    while (1) {
        display_text(100, 130, "Image 1/5", COLOR_WHITE);
        vTaskDelay(pdMS_TO_TICKS(100));
        
        display_text(80, 280, "< Swipe to change >", COLOR_WHITE);
        vTaskDelay(pdMS_TO_TICKS(100));
        
        display_text(10, 450, "Short:Change Mode Long:Net Config", COLOR_WHITE);
        vTaskDelay(pdMS_TO_TICKS(100));
        
        display_text(20, 12, "Mode: Gallery", COLOR_WHITE);
        vTaskDelay(pdMS_TO_TICKS(100));
        
        // 喂狗
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * @brief 应用程序入口
 */
void app_main(void)
{
    ESP_LOGI(TAG, "应用程序启动");
    
    // 初始化看门狗
    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = 5000,  // 5秒超时
        .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,  // 监控所有核心
        .trigger_panic = true,  // 超时时触发panic
    };
    ESP_ERROR_CHECK(esp_task_wdt_init(&twdt_config));
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL)); // 添加当前任务到看门狗
    
    // 初始化背光 - 这是关键步骤！
    init_backlight();
    
    // 初始化LCD显示
    esp_err_t ret = init_lcd();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LCD初始化失败: %s", esp_err_to_name(ret));
        
        // 即使LCD初始化失败，也闪烁背光作为指示
        while(1) {
            gpio_set_level(LCD_PIN_NUM_BL, 1);
            vTaskDelay(pdMS_TO_TICKS(500));
            gpio_set_level(LCD_PIN_NUM_BL, 0);
            vTaskDelay(pdMS_TO_TICKS(500));
            esp_task_wdt_reset();
        }
        return;
    }
    
    // 显示测试图案验证硬件
    display_test_pattern();
    
    // 创建显示任务
    xTaskCreate(display_task, "display_task", 4096, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "系统初始化完成，开始运行");
    
    // 主循环 - 定期喂狗
    while (1) {
        esp_task_wdt_reset(); // 喂狗
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}