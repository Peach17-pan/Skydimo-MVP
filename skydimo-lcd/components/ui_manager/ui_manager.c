#include "ui_manager.h"
#include "esp_lcd_axs15231b.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"  // 添加这个头文件
#include <string.h>

static const char *TAG = "ui_manager";

esp_lcd_panel_handle_t panel_handle = NULL;


// LCD配置
#define LCD_PIXEL_CLOCK_HZ     (20 * 1000 * 1000)
#define LCD_BK_LIGHT_ON_LEVEL  1
#define LCD_BK_LIGHT_OFF_LEVEL !LCD_BK_LIGHT_ON_LEVEL

#define LCD_HRES               320
#define LCD_VRES               240

// SPI引脚配置
#define PIN_NUM_SCLK           12
#define PIN_NUM_MOSI           11
#define PIN_NUM_MISO           -1
#define PIN_NUM_LCD_CS         10
#define PIN_NUM_LCD_DC         9
#define PIN_NUM_LCD_RST        5
#define PIN_NUM_LCD_BL         6
#define PIN_NUM_TOUCH_CS       4
#define PIN_NUM_TOUCH_RST      3

// 颜色定义 (RGB565)
#define COLOR_BLACK        0x0000
#define COLOR_WHITE        0xFFFF
#define COLOR_RED          0xF800
#define COLOR_GREEN        0x07E0
#define COLOR_BLUE         0x001F
#define COLOR_YELLOW       0xFFE0
#define COLOR_CYAN         0x07FF
#define COLOR_MAGENTA      0xF81F
#define COLOR_GRAY         0x8410
#define COLOR_DARK_GRAY    0x4208
#define COLOR_LIGHT_GRAY   0xC618

static esp_lcd_panel_io_handle_t io_handle = NULL;
//static esp_lcd_panel_io_handle_t touch_io_handle = NULL;

esp_err_t ui_manager_init(system_handle_t *sys_handle) {
    ESP_LOGI(TAG, "初始化UI管理器");
    
    esp_err_t ret;
    
    // SPI总线初始化
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_SCLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_HRES * LCD_VRES * sizeof(uint16_t),
    };
    ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI总线初始化失败");
        return ret;
    }
    
    // LCD IO控制器初始化
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_LCD_DC,
        .cs_gpio_num = PIN_NUM_LCD_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &io_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LCD IO控制器初始化失败");
        return ret;
    }
    
    // LCD面板初始化
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_LCD_RST,
        .color_space = ESP_LCD_COLOR_SPACE_RGB,
        .bits_per_pixel = 16,
    };
    ret = esp_lcd_new_panel_axs15231b(io_handle, &panel_config, &sys_handle->panel_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LCD面板初始化失败");
        return ret;
    }
    
    // 保存面板句柄到全局变量
    panel_handle = sys_handle->panel_handle;
    
    esp_lcd_panel_reset(sys_handle->panel_handle);
    esp_lcd_panel_init(sys_handle->panel_handle);
    esp_lcd_panel_disp_on_off(sys_handle->panel_handle, true);
    
    // 修复背光控制 - 使用正确的 GPIO 配置方式
    gpio_config_t bk_gpio_config = {
        .pin_bit_mask = (1ULL << PIN_NUM_LCD_BL),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ret = gpio_config(&bk_gpio_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "背光GPIO配置失败");
        return ret;
    }
    gpio_set_level(PIN_NUM_LCD_BL, LCD_BK_LIGHT_ON_LEVEL);
    
    sys_handle->display_initialized = true;
    ESP_LOGI(TAG, "LCD显示初始化完成");
    
    return ESP_OK;
}

void ui_show_current_mode(system_handle_t *sys_handle) {
    if (!sys_handle->display_initialized) {
        ESP_LOGI(TAG, "显示未初始化，跳过UI绘制");
        return;
    }
    
    // 简单清屏
    ui_clear_screen();
    
    // 根据模式显示不同内容
    switch (sys_handle->current_mode) {
        case MODE_WIFI:
            ui_draw_text_center("WiFi Mode", 80, COLOR_WHITE);
            break;
        case MODE_CLOCK:
            ui_draw_text_center("Clock Mode", 80, COLOR_WHITE);
            break;
        case MODE_WEATHER:
            ui_draw_text_center("Weather Mode", 80, COLOR_WHITE);
            break;
        case MODE_GALLERY:
            ui_draw_text_center("Gallery Mode", 80, COLOR_WHITE);
            break;
        case MODE_KEYBOARD:
            ui_draw_text_center("Keyboard Mode", 80, COLOR_WHITE);
            break;
        default:
            break;
    }
    
    sys_handle->mode_start_time = xTaskGetTickCount();
}

void ui_clear_screen(void) {
    if (!panel_handle) return;
    
    // 简单的清屏实现 - 填充黑色
    uint16_t black_color = 0x0000; // RGB565 黑色
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_HRES, LCD_VRES, &black_color);
}

void ui_draw_text_center(const char *text, uint16_t y, uint16_t color) {
    // 简化实现 - 在实际项目中需要实现字符绘制
    // 这里只是示例，实际使用时需要真正的文本渲染
    ESP_LOGI(TAG, "绘制文本: %s", text);
}

void ui_draw_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color) {
    if (!panel_handle) return;
    
    // 简化实现 - 绘制单色矩形
    uint16_t *line_buffer = malloc(width * sizeof(uint16_t));
    if (line_buffer) {
        for (int i = 0; i < width; i++) {
            line_buffer[i] = color;
        }
        for (int row = 0; row < height; row++) {
            esp_lcd_panel_draw_bitmap(panel_handle, x, y + row, x + width, y + row + 1, line_buffer);
        }
        free(line_buffer);
    }
}

void ui_handle_touch(system_handle_t *sys_handle, uint16_t x, uint16_t y) {
    // 暂时不实现触摸功能
    ESP_LOGI(TAG, "触摸事件: (%d, %d)", x, y);
}

void ui_handle_long_press(system_handle_t *sys_handle) {
    // 长按操作处理
    ESP_LOGI(TAG, "长按操作 - 当前模式: %d", sys_handle->current_mode);
}

void ui_update_status_display(system_handle_t *sys_handle) {
    // 更新状态信息显示
    uint32_t uptime = (xTaskGetTickCount() - sys_handle->mode_start_time) / 1000;
    ESP_LOGI(TAG, "模式运行时间: %lu秒", uptime);
}