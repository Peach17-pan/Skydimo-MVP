#include "ui_port.h"
#include "lcd_touch.h"
#include "mode_manager.h"
#include "esp_log.h"

static const char *TAG = "UI";

static const char *mode_name[] = {
    "时钟模式", 
    "天气模式", 
    "相册模式", 
    "键盘模式", 
    "配网模式"
};

static const uint16_t mode_bg_color[] = {
    0x0000,  // 时钟 - 黑色背景
    0x001F,  // 天气 - 蓝色背景  
    0xF800,  // 相册 - 红色背景
    0x07E0,  // 键盘 - 绿色背景
    0xFFE0   // 配网 - 黄色背景
};

/*
static const uint16_t mode_text_color[] = {
    0xFFFF,  // 时钟 - 白色文字
    0xFFFF,  // 天气 - 白色文字
    0xFFFF,  // 相册 - 白色文字  
    0x0000,  // 键盘 - 黑色文字
    0x0000   // 配网 - 黑色文字
};*/

void ui_switch(mode_t m)
{
    if (m >= MODE_MAX) {
        ESP_LOGE(TAG, "无效的模式: %d", m);
        return;
    }
    
    ESP_LOGI(TAG, "切换到 %s", mode_name[m]);
    
    // 填充背景色
    lcd_fill_color(mode_bg_color[m]);
    
    // 显示模式信息,简化处理
    lcd_print_mode(mode_name[m]);
    
   
}