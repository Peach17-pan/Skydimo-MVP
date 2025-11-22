#include "lcd_touch.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"  // 只保留核心操作
#include "esp_log.h"
#include "esp_heap_caps.h"

#define LCD_HOST        SPI2_HOST
#define LCD_MOSI        11
#define LCD_CLK         12
#define LCD_CS          10
#define LCD_DC          13
#define LCD_RST         9
#define LCD_BACKLIGHT   48

#define LCD_WIDTH       480
#define LCD_HEIGHT      800

static esp_lcd_panel_io_handle_t io_handle = NULL;

// 手动发送GC9503初始化命令
static void gc9503_manual_init(void)
{
    const struct { uint8_t cmd; uint8_t data[16]; uint8_t len; } cmds[] = {
        {0xFE, {0x00}, 1},
        {0xEF, {0x00}, 1},
        {0x36, {0x00}, 1},  // Memory Access Control
        {0x3A, {0x55}, 1},  // 16-bit color
        {0xB4, {0x10}, 1},  // Inversion control
        {0xC2, {0x33}, 1},  // Power control
        {0x11, {0x00}, 0},  // Sleep out
    };
    
    for (int i = 0; i < sizeof(cmds)/sizeof(cmds[0]); i++) {
        esp_lcd_panel_io_tx_param(io_handle, cmds[i].cmd, cmds[i].len ? cmds[i].data : NULL, cmds[i].len);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    vTaskDelay(pdMS_TO_TICKS(120));
    esp_lcd_panel_io_tx_param(io_handle, 0x29, NULL, 0);  // Display ON
}

void lcd_touch_init(void)
{
    ESP_LOGI("LCD", "=== GC9503 手动初始化模式 ===");
    
    // 1. 控制引脚
    gpio_reset_pin(LCD_BACKLIGHT);
    gpio_set_direction(LCD_BACKLIGHT, GPIO_MODE_OUTPUT);
    gpio_set_level(LCD_BACKLIGHT, 1);
    
    gpio_reset_pin(LCD_RST);
    gpio_set_direction(LCD_RST, GPIO_MODE_OUTPUT);
    gpio_set_level(LCD_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(LCD_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    // 2. SPI总线
    spi_bus_config_t buscfg = {
        .sclk_io_num = LCD_CLK,
        .mosi_io_num = LCD_MOSI,
        .miso_io_num = -1,
        .max_transfer_sz = LCD_WIDTH * LCD_HEIGHT * 2 + 8
    };
    spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);

    // 3. IO句柄（关键：不依赖具体驱动）
    esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = LCD_CS,
        .dc_gpio_num = LCD_DC,
        .spi_mode = 0,
        .pclk_hz = 10 * 1000 * 1000,
        .trans_queue_depth = 10,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle);

    // 4. 手动初始化屏幕
    gc9503_manual_init();

    ESP_LOGI("LCD", "=== 初始化完成 ===");
}

void lcd_fill_color(uint16_t color)
{
    if (!io_handle) {
        ESP_LOGE("LCD", "IO句柄为空！");
        return;
    }

    // 设置写区域（全屏）
    esp_lcd_panel_io_tx_param(io_handle, 0x2A, (uint8_t[]){0, 0, LCD_WIDTH >> 8, LCD_WIDTH & 0xFF}, 4);
    esp_lcd_panel_io_tx_param(io_handle, 0x2B, (uint8_t[]){0, 0, LCD_HEIGHT >> 8, LCD_HEIGHT & 0xFF}, 4);
    esp_lcd_panel_io_tx_param(io_handle, 0x2C, NULL, 0);  // Memory write

    // 分配DMA内存
    uint16_t *line = heap_caps_malloc(LCD_WIDTH * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (!line) {
        ESP_LOGE("LCD", "DMA内存分配失败！");
        return;
    }

    for (int x = 0; x < LCD_WIDTH; x++) line[x] = color;

    // 批量发送（800行）
    for (int y = 0; y < LCD_HEIGHT; y++) {
        esp_lcd_panel_io_tx_color(io_handle, -1, line, LCD_WIDTH * sizeof(uint16_t));
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    free(line);
    ESP_LOGI("LCD", "颜色填充完成: 0x%04X", color);
}

void* lcd_get_panel_handle(void)
{
    return (void*)io_handle;  // 返回IO句柄即可
}

void lcd_print_mode(const char *name)
{
    ESP_LOGI("UI", "====================");
    ESP_LOGI("UI", "模式: %s", name);
    ESP_LOGI("UI", "====================");
}