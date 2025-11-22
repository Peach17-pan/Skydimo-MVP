#include "ui_mode.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "UI_MODE";

// 屏幕对象
static lv_obj_t *current_screen = NULL;
static lv_obj_t *screens[MODE_MAX] = {NULL};

void ui_init(void)
{
    ESP_LOGI(TAG, "UI module initializing...");
    
    // 创建所有屏幕
    ui_create_netcfg_screen();
    ui_create_clock_screen();
    ui_create_weather_screen();
    ui_create_gallery_screen();
    ui_create_vkeyboard_screen();
    
    // 默认显示配网模式
    ui_switch_mode(MODE_NET_CFG);
    
    ESP_LOGI(TAG, "UI module initialized");
}

void ui_create_netcfg_screen(void)
{
    screens[MODE_NET_CFG] = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screens[MODE_NET_CFG], lv_color_hex(0x2196F3), LV_PART_MAIN);
    
    lv_obj_t *title = lv_label_create(screens[MODE_NET_CFG]);
    lv_label_set_text(title, "配网模式");
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);
    
    lv_obj_t *desc = lv_label_create(screens[MODE_NET_CFG]);
    lv_label_set_text(desc, "长按按键进入配网\n等待网络配置...");
    lv_obj_set_style_text_color(desc, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(desc, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(desc, LV_ALIGN_CENTER, 0, 0);
}

void ui_create_clock_screen(void)
{
    screens[MODE_CLOCK] = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screens[MODE_CLOCK], lv_color_hex(0x000000), LV_PART_MAIN);
    
    lv_obj_t *time_label = lv_label_create(screens[MODE_CLOCK]);
    lv_label_set_text(time_label, "12:00:00");
    lv_obj_set_style_text_color(time_label, lv_color_hex(0x00FF00), LV_PART_MAIN);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, -20);
}

void ui_create_weather_screen(void)
{
    screens[MODE_WEATHER] = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screens[MODE_WEATHER], lv_color_hex(0x87CEEB), LV_PART_MAIN);
    
    lv_obj_t *title = lv_label_create(screens[MODE_WEATHER]);
    lv_label_set_text(title, "天气信息");
    lv_obj_set_style_text_color(title, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);
    
    lv_obj_t *info = lv_label_create(screens[MODE_WEATHER]);
    lv_label_set_text(info, "城市: 北京\n温度: 25°C\n天气: 晴");
    lv_obj_set_style_text_color(info, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_text_align(info, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(info, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(info, LV_ALIGN_CENTER, 0, 0);
}

void ui_create_gallery_screen(void)
{
    screens[MODE_GALLERY] = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screens[MODE_GALLERY], lv_color_hex(0x2E2E2E), LV_PART_MAIN);
    
    lv_obj_t *title = lv_label_create(screens[MODE_GALLERY]);
    lv_label_set_text(title, "相册模式");
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);
    
    lv_obj_t *hint = lv_label_create(screens[MODE_GALLERY]);
    lv_label_set_text(hint, "触摸左右滑动切换图片");
    lv_obj_set_style_text_color(hint, lv_color_hex(0xAAAAAA), LV_PART_MAIN);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -20);
}

void ui_create_vkeyboard_screen(void)
{
    screens[MODE_VKEYBOARD] = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screens[MODE_VKEYBOARD], lv_color_hex(0x4CAF50), LV_PART_MAIN);
    
    lv_obj_t *title = lv_label_create(screens[MODE_VKEYBOARD]);
    lv_label_set_text(title, "虚拟键盘");
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    lv_obj_t *status = lv_label_create(screens[MODE_VKEYBOARD]);
    lv_label_set_text(status, "点击按键发送指令到PC");
    lv_obj_set_style_text_color(status, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(status, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(status, LV_ALIGN_BOTTOM_MID, 0, -20);
}

void ui_switch_mode(sys_mode_t mode)
{
    if (mode >= MODE_MAX) {
        ESP_LOGE(TAG, "Invalid mode: %d", mode);
        return;
    }
    
    if (screens[mode] == NULL) {
        ESP_LOGE(TAG, "Screen not created for mode %d", mode);
        return;
    }
    
    if (screens[mode] != current_screen) {
        lv_scr_load(screens[mode]);
        current_screen = screens[mode];
        ESP_LOGI(TAG, "Switched to mode %d", mode);
    }
}