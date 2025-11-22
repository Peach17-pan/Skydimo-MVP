#include "lcd_driver.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>  // 添加 string.h 头文件
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "LCD_DRIVER";
static spi_device_handle_t spi_handle;

// 定义DC引脚（如果没有定义的话）
#ifndef EXAMPLE_PIN_NUM_LCD_DC
#define EXAMPLE_PIN_NUM_LCD_DC (GPIO_NUM_8)  // 根据实际硬件调整
#endif

// SPI预传输回调，用于设置DC引脚
static void lcd_spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc = (int)t->user;
    gpio_set_level(EXAMPLE_PIN_NUM_LCD_DC, dc);
}

void lcd_driver_init(void) {
    ESP_LOGI(TAG, "Initializing LCD driver");

    // 初始化DC引脚
    gpio_config_t dc_io_conf = {
        .pin_bit_mask = (1ULL << EXAMPLE_PIN_NUM_LCD_DC),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&dc_io_conf);
    gpio_set_level(EXAMPLE_PIN_NUM_LCD_DC, 0);

    // 初始化RST引脚
    gpio_config_t rst_io_conf = {
        .pin_bit_mask = (1ULL << EXAMPLE_PIN_NUM_LCD_RST),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&rst_io_conf);
    gpio_set_level(EXAMPLE_PIN_NUM_LCD_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(EXAMPLE_PIN_NUM_LCD_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    //执行初始化序列
    lcd_init_sequence();

    spi_bus_config_t buscfg = {
        .miso_io_num = -1,
        .mosi_io_num = EXAMPLE_PIN_NUM_LCD_DATA0,
        .sclk_io_num = EXAMPLE_PIN_NUM_LCD_PCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = EXAMPLE_LCD_H_RES * 64 * 2,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 40 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = EXAMPLE_PIN_NUM_LCD_CS,
        .queue_size = 7,
        .pre_cb = lcd_spi_pre_transfer_callback, // 设置预传输回调
    };

    ESP_ERROR_CHECK(spi_bus_add_device(LCD_HOST, &devcfg, &spi_handle));

    // 执行LCD初始化序列
    lcd_init_sequence();

    ESP_LOGI(TAG, "LCD driver initialized");
}

// 发送命令
void lcd_send_command(uint8_t cmd) {
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &cmd;
    t.user = (void*)0;  // DC=0 表示命令
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi_handle, &t));
}

// 发送数据
void lcd_send_data(void *data, size_t len) {
    if (len == 0) return;
    
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = len * 8;
    t.tx_buffer = data;
    t.user = (void*)1;  // DC=1 表示数据
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi_handle, &t));
}

void lcd_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    uint8_t data[4];
    
    // 设置列地址
    lcd_send_command(0x2A);
    data[0] = (x1 >> 8) & 0xFF;
    data[1] = x1 & 0xFF;
    data[2] = (x2 >> 8) & 0xFF;
    data[3] = x2 & 0xFF;
    lcd_send_data(data, 4);
    
    // 设置行地址
    lcd_send_command(0x2B);
    data[0] = (y1 >> 8) & 0xFF;
    data[1] = y1 & 0xFF;
    data[2] = (y2 >> 8) & 0xFF;
    data[3] = y2 & 0xFF;
    lcd_send_data(data, 4);
    
    // 开始写入显存
    lcd_send_command(0x2C);
}

void lcd_init_sequence(void) {
    ESP_LOGI(TAG, "Starting LCD initialization sequence");
    
    // 更完整的ST7789初始化序列
    lcd_send_command(0x01); // Software reset
    vTaskDelay(pdMS_TO_TICKS(150));
    
    lcd_send_command(0x11); // Sleep out
    vTaskDelay(pdMS_TO_TICKS(120));
    
    lcd_send_command(0x3A); // Interface Pixel Format
    uint8_t pixfmt = 0x55;  // 16-bit RGB565
    lcd_send_data(&pixfmt, 1);
    
    lcd_send_command(0x36); // Memory Data Access Control
    uint8_t madctl = 0x00;
#if (Rotated == USER_DISP_ROT_90)
    madctl = 0x60; // MY=1, MX=1, MV=1 (旋转90度)
#else
    madctl = 0x00; // 正常方向
#endif
    lcd_send_data(&madctl, 1);
    
    lcd_send_command(0x2A); // Column Address Set
    uint8_t col_data[4] = {0x00, 0x00, (EXAMPLE_LCD_H_RES >> 8) & 0xFF, EXAMPLE_LCD_H_RES & 0xFF};
    lcd_send_data(col_data, 4);
    
    lcd_send_command(0x2B); // Row Address Set
    uint8_t row_data[4] = {0x00, 0x00, (EXAMPLE_LCD_V_RES >> 8) & 0xFF, EXAMPLE_LCD_V_RES & 0xFF};
    lcd_send_data(row_data, 4);
    
    lcd_send_command(0x21); // Display Inversion On
    lcd_send_command(0x13); // Normal Display Mode On
    lcd_send_command(0x29); // Display on
    
    vTaskDelay(pdMS_TO_TICKS(120));
    ESP_LOGI(TAG, "LCD initialization sequence completed");
}
//添加测试函数
void lcd_test_pattern(void)
{
    ESP_LOGI(TAG, "Starting LCD test pattern");
    
    // 填充红色
    lcd_fill_color(0xF800); // 红色
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // 填充绿色
    lcd_fill_color(0x07E0); // 绿色
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // 填充蓝色
    lcd_fill_color(0x001F); // 蓝色
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // 填充白色
    lcd_fill_color(0xFFFF); // 白色
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // 填充黑色
    lcd_fill_color(0x0000); // 黑色
    
    ESP_LOGI(TAG, "LCD test pattern completed");
}

// 添加颜色填充函数
void lcd_fill_color(uint16_t color)
{
    uint32_t size = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES;
    uint8_t color_hi = (color >> 8) & 0xFF;
    uint8_t color_lo = color & 0xFF;
    
    // 设置显示区域为全屏
    lcd_set_window(0, 0, EXAMPLE_LCD_H_RES - 1, EXAMPLE_LCD_V_RES - 1);
    
    // 开始写入显存
    lcd_send_command(0x2C);
    
    // 发送颜色数据
    for (uint32_t i = 0; i < size; i++) {
        lcd_send_data(&color_hi, 1);
        lcd_send_data(&color_lo, 1);
    }
}
// 在 lcd_driver.c 中添加
void lcd_detailed_test(void)
{
    ESP_LOGI(TAG, "=== Detailed LCD Test ===");
    
    // 测试1: 全屏红色
    ESP_LOGI(TAG, "Test 1: Full screen RED");
    lcd_fill_color(0xF800);
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // 测试2: 全屏绿色
    ESP_LOGI(TAG, "Test 2: Full screen GREEN");
    lcd_fill_color(0x07E0);
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // 测试3: 全屏蓝色
    ESP_LOGI(TAG, "Test 3: Full screen BLUE");
    lcd_fill_color(0x001F);
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // 测试4: 绘制彩色条纹
    ESP_LOGI(TAG, "Test 4: Color stripes");
    lcd_draw_stripes();
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    // 测试5: 绘制测试图案
    ESP_LOGI(TAG, "Test 5: Test pattern");
    lcd_draw_test_pattern();
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    ESP_LOGI(TAG, "Detailed LCD test completed");
}

void lcd_draw_stripes(void)
{
    uint16_t colors[] = {0xF800, 0x07E0, 0x001F, 0xFFFF, 0x0000}; // 红绿蓝白黑
    int stripe_width = EXAMPLE_LCD_H_RES / 5;
    
    for (int i = 0; i < 5; i++) {
        lcd_set_window(i * stripe_width, 0, (i + 1) * stripe_width - 1, EXAMPLE_LCD_V_RES - 1);
        lcd_send_command(0x2C); // Memory write
        
        uint8_t color_hi = (colors[i] >> 8) & 0xFF;
        uint8_t color_lo = colors[i] & 0xFF;
        uint32_t pixels = stripe_width * EXAMPLE_LCD_V_RES;
        
        for (uint32_t j = 0; j < pixels; j++) {
            lcd_send_data(&color_hi, 1);
            lcd_send_data(&color_lo, 1);
        }
    }
}

void lcd_draw_test_pattern(void)
{
    // 填充黑色背景
    lcd_fill_color(0x0000);
    
    // 绘制白色边框
    lcd_draw_rect(0, 0, EXAMPLE_LCD_H_RES - 1, EXAMPLE_LCD_V_RES - 1, 0xFFFF);
    
    // 绘制交叉线
    lcd_draw_line(0, 0, EXAMPLE_LCD_H_RES - 1, EXAMPLE_LCD_V_RES - 1, 0xF800); // 红色对角线
    lcd_draw_line(EXAMPLE_LCD_H_RES - 1, 0, 0, EXAMPLE_LCD_V_RES - 1, 0x07E0); // 绿色对角线
    
    // 绘制中心方块
    int center_x = EXAMPLE_LCD_H_RES / 2;
    int center_y = EXAMPLE_LCD_V_RES / 2;
    int size = 50;
    lcd_draw_rect(center_x - size, center_y - size, center_x + size, center_y + size, 0x001F); // 蓝色方块
}

void lcd_draw_rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    lcd_set_window(x1, y1, x2, y2);
    lcd_send_command(0x2C); // Memory write
    
    uint8_t color_hi = (color >> 8) & 0xFF;
    uint8_t color_lo = color & 0xFF;
    uint32_t pixels = (x2 - x1 + 1) * (y2 - y1 + 1);
    
    for (uint32_t i = 0; i < pixels; i++) {
        lcd_send_data(&color_hi, 1);
        lcd_send_data(&color_lo, 1);
    }
}

void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    // 简化版的Bresenham直线算法
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    
    while (1) {
        lcd_draw_pixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    lcd_set_window(x, y, x, y);
    lcd_send_command(0x2C); // Memory write
    
    uint8_t color_data[2] = {(color >> 8) & 0xFF, color & 0xFF};
    lcd_send_data(color_data, 2);
}