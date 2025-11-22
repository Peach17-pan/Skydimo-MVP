#include "lcd_touch.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
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

// 完整的GC9503初始化命令序列
static void gc9503_manual_init(void)
{
    ESP_LOGI("LCD", "开始GC9503初始化...");
    
    // 完整的初始化序列
    const struct { 
        uint8_t cmd; 
        uint8_t data[16]; 
        uint8_t len; 
        uint16_t delay; 
    } cmds[] = {
        {0xFE, {0x00}, 1, 10},        // 进入扩展命令模式
        {0xEF, {0x00}, 1, 10},        // 进入扩展命令模式2
        {0x35, {0x00}, 1, 10},        // TEON: Tearing Effect Line ON
        {0x36, {0x00}, 1, 10},        // MADCTL: Memory Data Access Control
        {0x3A, {0x55}, 1, 10},        // COLMOD: Interface Pixel Format - 16bit/pixel
        {0x84, {0x40}, 1, 10},        // DSI write mode setting
        {0xB1, {0x00, 0x1F}, 2, 10},  // FRMCTR1: Frame Rate Control
        {0xB2, {0x00, 0x1F}, 2, 10},  // FRMCTR2: Frame Rate Control
        {0xB3, {0x00, 0x1F}, 2, 10},  // FRMCTR3: Frame Rate Control
        {0xB4, {0x10}, 1, 10},        // INVCTR: Display Inversion Control
        {0xC0, {0x3E, 0x3E}, 2, 10},  // PWCTR1: Power Control 1
        {0xC1, {0x06}, 1, 10},        // PWCTR2: Power Control 2
        {0xC2, {0x01}, 1, 10},        // PWCTR3: Power Control 3
        {0xC5, {0x30}, 1, 10},        // VMCTR1: VCOM Control 1
        {0xFC, {0x08}, 1, 10},        // Command Set Control
        {0xE8, {0x12, 0x00}, 2, 10},  // EQCTRL: Equalize time control
        {0xE9, {0x32, 0x02}, 2, 10},  // EQCTRL2
        {0xEA, {0x12, 0x00}, 2, 10},  // EQCTRL3
        {0xEB, {0x32, 0x02}, 2, 10},  // EQCTRL4
        {0xEC, {0x12, 0x00}, 2, 10},  // EQCTRL5
        {0xED, {0x32, 0x02}, 2, 10},  // EQCTRL6
        {0x26, {0x1D}, 1, 10},        // GAMMA: Gamma curve selected
        {0x2A, {0x00, 0x00, 0x01, 0xDF}, 4, 10}, // CASET: Column address set
        {0x2B, {0x00, 0x00, 0x03, 0x1F}, 4, 10}, // RASET: Row address set
        {0x11, {0x00}, 0, 120},       // SLPOUT: Sleep out
        {0x29, {0x00}, 0, 120},       // DISPON: Display on
    };
    
    for (int i = 0; i < sizeof(cmds)/sizeof(cmds[0]); i++) {
        if (cmds[i].len > 0) {
            esp_lcd_panel_io_tx_param(io_handle, cmds[i].cmd, cmds[i].data, cmds[i].len);
        } else {
            esp_lcd_panel_io_tx_param(io_handle, cmds[i].cmd, NULL, 0);
        }
        if (cmds[i].delay > 0) {
            vTaskDelay(pdMS_TO_TICKS(cmds[i].delay));
        }
    }
    
    ESP_LOGI("LCD", "GC9503初始化完成");
}

void lcd_touch_init(void)
{
    ESP_LOGI("LCD", "=== GC9503 初始化开始 ===");
    
    // 1. 硬件复位
    gpio_reset_pin(LCD_RST);
    gpio_set_direction(LCD_RST, GPIO_MODE_OUTPUT);
    gpio_set_level(LCD_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(LCD_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // 背光控制
    gpio_reset_pin(LCD_BACKLIGHT);
    gpio_set_direction(LCD_BACKLIGHT, GPIO_MODE_OUTPUT);
    gpio_set_level(LCD_BACKLIGHT, 1);

    // 2. SPI总线初始化
    spi_bus_config_t buscfg = {
        .sclk_io_num = LCD_CLK,
        .mosi_io_num = LCD_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_WIDTH * LCD_HEIGHT * 2 + 8
    };
    esp_err_t ret = spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE("LCD", "SPI总线初始化失败: %s", esp_err_to_name(ret));
        return;
    }

    // 3. LCD IO配置 - 使用SPI模式3
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = LCD_DC,
        .cs_gpio_num = LCD_CS,
        .pclk_hz = 40 * 1000 * 1000,  // 40MHz
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 3,  // GC9503通常使用模式3
        .trans_queue_depth = 10,
    };
    
    ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle);
    if (ret != ESP_OK) {
        ESP_LOGE("LCD", "LCD IO初始化失败: %s", esp_err_to_name(ret));
        return;
    }

    // 4. 手动初始化屏幕
    gc9503_manual_init();

    ESP_LOGI("LCD", "=== LCD初始化完成 ===");
}

void lcd_fill_color(uint16_t color)
{
    if (!io_handle) {
        ESP_LOGE("LCD", "IO句柄为空！");
        return;
    }

    // 设置写区域（全屏）
    uint8_t caset_data[] = {0x00, 0x00, (LCD_WIDTH >> 8) & 0xFF, LCD_WIDTH & 0xFF};
    uint8_t raset_data[] = {0x00, 0x00, (LCD_HEIGHT >> 8) & 0xFF, LCD_HEIGHT & 0xFF};
    
    esp_lcd_panel_io_tx_param(io_handle, 0x2A, caset_data, 4);  // CASET
    esp_lcd_panel_io_tx_param(io_handle, 0x2B, raset_data, 4);  // RASET
    esp_lcd_panel_io_tx_param(io_handle, 0x2C, NULL, 0);        // RAMWR

    // 分配一行像素的缓冲区
    uint16_t *line_buffer = heap_caps_malloc(LCD_WIDTH * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (!line_buffer) {
        ESP_LOGE("LCD", "DMA内存分配失败！");
        return;
    }

    // 填充一行数据
    for (int x = 0; x < LCD_WIDTH; x++) {
        line_buffer[x] = color;
    }

    // 逐行发送数据
    for (int y = 0; y < LCD_HEIGHT; y++) {
        esp_lcd_panel_io_tx_color(io_handle, -1, line_buffer, LCD_WIDTH * sizeof(uint16_t));
    }
    
    free(line_buffer);
    ESP_LOGI("LCD", "屏幕填充完成: 0x%04X", color);
}

// 其他函数保持不变...
void* lcd_get_panel_handle(void)
{
    return (void*)io_handle;
}

void lcd_print_mode(const char *name)
{
    ESP_LOGI("UI", "====================");
    ESP_LOGI("UI", "模式: %s", name);
    ESP_LOGI("UI", "====================");
}