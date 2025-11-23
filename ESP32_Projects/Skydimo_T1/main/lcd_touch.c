#include "lcd_touch.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LCD_HOST        SPI2_HOST
#define LCD_PCLK        12
#define LCD_MOSI        11
#define LCD_CS          10
#define LCD_DC          14
#define LCD_RST         15
#define LCD_BL          13

#define LCD_WIDTH       320
#define LCD_HEIGHT      480

static const char *TAG = "LCD";
static esp_lcd_panel_io_handle_t io_handle = NULL;

// 完整的GC9503初始化序列
static const uint8_t gc9503_init_commands[] = {
    // 软件复位
    0x01, 0x80, 150,
    // 退出睡眠模式
    0x11, 0x80, 120,
    // 像素格式设置 - RGB565
    0x3A, 1, 0x55,
    // 内存访问控制
    0x36, 1, 0x00,
    // 帧率控制
    0xB1, 3, 0x00, 0x1F, 0x0F,
    // 显示反转关闭
    0x20, 0x80, 10,
    // 电源控制1
    0xC0, 2, 0x1B, 0x1B,
    // 电源控制2
    0xC1, 1, 0x45,
    // VCOM控制
    0xC5, 2, 0x38, 0x80,
    // 正伽马校正
    0xE0, 15, 0x0F, 0x26, 0x24, 0x0B, 0x0E, 0x09, 0x54, 0xA8, 
              0x46, 0x0C, 0x17, 0x09, 0x0F, 0x07, 0x00,
    // 负伽马校正
    0xE1, 15, 0x00, 0x19, 0x1B, 0x04, 0x11, 0x06, 0x2A, 0x56,
              0x39, 0x03, 0x28, 0x06, 0x10, 0x08, 0x0F,
    // 设置列地址 (0-319)
    0x2A, 4, 0x00, 0x00, 0x01, 0x3F,
    // 设置行地址 (0-479)
    0x2B, 4, 0x00, 0x00, 0x01, 0xDF,
    // 开启显示
    0x29, 0x80, 120,
    0x00 // 结束标记
};

static void gc9503_send_command(uint8_t cmd, const uint8_t *data, size_t len)
{
    // 发送命令
    esp_lcd_panel_io_tx_param(io_handle, cmd, NULL, 0);
    
    // 如果有数据，发送数据
    if (data && len > 0) {
        for (size_t i = 0; i < len; i++) {
            uint8_t temp = data[i];
            esp_lcd_panel_io_tx_param(io_handle, 0x00, &temp, 1);
        }
    }
}

static void gc9503_init(void)
{
    ESP_LOGI(TAG, "开始GC9503初始化");
    
    int idx = 0;
    while (gc9503_init_commands[idx] != 0x00) {
        uint8_t cmd = gc9503_init_commands[idx++];
        uint8_t num_args = gc9503_init_commands[idx++];
        
        if (num_args == 0x80) {
            // 只有延迟的命令
            uint8_t delay_ms = gc9503_init_commands[idx++];
            gc9503_send_command(cmd, NULL, 0);
            if (delay_ms > 0) {
                vTaskDelay(pdMS_TO_TICKS(delay_ms));
            }
        } else {
            // 带参数的命令
            gc9503_send_command(cmd, &gc9503_init_commands[idx], num_args);
            idx += num_args;
        }
    }
    
    ESP_LOGI(TAG, "GC9503初始化完成");
}

void lcd_touch_init(void)
{
    ESP_LOGI(TAG, "=== LCD初始化开始 ===");
    
    // 1. 配置GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LCD_RST) | (1ULL << LCD_BL),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    
    // 2. 硬件复位
    ESP_LOGI(TAG, "执行硬件复位");
    gpio_set_level(LCD_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(LCD_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(150));
    
    // 3. 开启背光
    gpio_set_level(LCD_BL, 1);
    ESP_LOGI(TAG, "背光开启");

    // 4. SPI总线配置
    ESP_LOGI(TAG, "配置SPI总线");
    spi_bus_config_t buscfg = {
        .sclk_io_num = LCD_PCLK,
        .mosi_io_num = LCD_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_WIDTH * 2, // 一行数据
        .flags = 0,
        .intr_flags = 0,
    };
    
    esp_err_t ret = spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI总线初始化失败: %s", esp_err_to_name(ret));
        return;
    }

    // 5. LCD IO配置
    ESP_LOGI(TAG, "配置LCD IO");
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = LCD_DC,
        .cs_gpio_num = LCD_CS,
        .pclk_hz = 20 * 1000 * 1000,  // 20MHz
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,  // 重要：尝试模式3
        .trans_queue_depth = 10,
        .on_color_trans_done = NULL,
        .user_ctx = NULL,
    };
    
    ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LCD IO初始化失败: %s", esp_err_to_name(ret));
        return;
    }

    // 6. 初始化GC9503
    gc9503_init();
    
    ESP_LOGI(TAG, "=== LCD初始化完成 ===");
}

void lcd_fill_color(uint16_t color)
{
    if (!io_handle) {
        ESP_LOGE(TAG, "IO句柄为空");
        return;
    }

    ESP_LOGI(TAG, "填充颜色: 0x%04X", color);
    
    // 设置写区域（全屏）
    uint8_t caset[] = {0x00, 0x00, 0x01, 0x3F}; // 0-319
    uint8_t raset[] = {0x00, 0x00, 0x01, 0xDF}; // 0-479
    
    esp_lcd_panel_io_tx_param(io_handle, 0x2A, caset, 4);
    esp_lcd_panel_io_tx_param(io_handle, 0x2B, raset, 4);
    
    // 发送内存写命令
    esp_lcd_panel_io_tx_param(io_handle, 0x2C, NULL, 0);

    // 分配缓冲区并填充颜色
    const int buffer_size = 320; // 一行像素
    uint16_t *line_buffer = malloc(buffer_size * sizeof(uint16_t));
    if (!line_buffer) {
        ESP_LOGE(TAG, "内存分配失败");
        return;
    }
    
    // 填充缓冲区
    for (int i = 0; i < buffer_size; i++) {
        line_buffer[i] = color;
    }
    
    // 逐行发送数据
    for (int y = 0; y < LCD_HEIGHT; y++) {
        esp_lcd_panel_io_tx_color(io_handle, 0x2C, line_buffer, buffer_size * sizeof(uint16_t));
    }
    
    free(line_buffer);
    ESP_LOGI(TAG, "颜色填充完成");
}

void lcd_draw_rect(int x, int y, int width, int height, uint16_t color)
{
    // 简化的矩形绘制
    lcd_fill_color(color);
}

void* lcd_get_panel_handle(void)
{
    return (void*)io_handle;
}

void lcd_print_mode(const char *name)
{
    ESP_LOGI("UI", "模式: %s", name);
}