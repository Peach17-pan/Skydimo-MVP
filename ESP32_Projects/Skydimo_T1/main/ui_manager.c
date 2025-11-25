#include "ui_manager.h"
#include "mode_manager.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "ui_manager";

static lv_display_t* main_display = NULL;
static lv_obj_t* current_screen = NULL;
static lv_obj_t* mode_label = NULL;
static lv_obj_t* mode_screens[MODE_MAX] = {0};

// 创建最简单的 UI
static void create_simple_ui(device_mode_t mode)
{
    lv_obj_t* screen = lv_obj_create(NULL);
    mode_screens[mode] = screen;
    
    // 设置背景色
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    
    // 创建模式标签
    mode_label = lv_label_create(screen);
    lv_obj_set_style_text_color(mode_label, lv_color_white(), 0);
    lv_obj_center(mode_label);
    
    // 根据模式设置不同的显示内容
    switch(mode) {
        case MODE_NETWORK_CONFIG:
            lv_label_set_text(mode_label, "📶 配网模式\n\n短按切换模式\n长按进入配网");
            break;
        case MODE_CLOCK:
            lv_label_set_text(mode_label, "🕒 时钟模式\n\n12:00:00\n2024-01-01");
            break;
        case MODE_WEATHER:
            lv_label_set_text(mode_label, "☀️ 天气模式\n\n25°C 晴朗\n北京市");
            break;
        case MODE_GALLERY:
            lv_label_set_text(mode_label, "🖼️ 相册模式\n\n图片浏览功能");
            break;
        case MODE_VIRTUAL_KEYBOARD:
            lv_label_set_text(mode_label, "⌨️ 键盘模式\n\n远程控制功能");
            break;
        default:
            lv_label_set_text(mode_label, "未知模式");
            break;
    }
}

void ui_manager_init(lv_display_t* display)
{
    main_display = display;
    
    // 创建所有模式的简单UI界面
    for (int i = 0; i < MODE_MAX; i++) {
        create_simple_ui((device_mode_t)i);
    }
    
    ESP_LOGI(TAG, "Simple UI manager initialized with %d modes", MODE_MAX);
}

void ui_update_current_mode(device_mode_t mode)
{
    if (mode >= MODE_MAX) {
        ESP_LOGE(TAG, "Invalid mode: %d", mode);
        return;
    }
    
    if (mode_screens[mode] != NULL) {
        lv_scr_load(mode_screens[mode]);
        current_screen = mode_screens[mode];
        ESP_LOGI(TAG, "UI switched to %s", get_mode_name(mode));
    }
}

// 其他UI创建函数（暂时为空实现）
void ui_create_network_config_screen(void) {}
void ui_create_clock_screen(void) {}
void ui_create_weather_screen(void) {}
void ui_create_gallery_screen(void) {}
void ui_create_virtual_keyboard_screen(void) {}