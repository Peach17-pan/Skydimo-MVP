#include "ui_manager.h"
#include "mode_manager.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "ui_manager";

static lv_display_t* main_display = NULL;
static lv_obj_t* current_screen = NULL;
static lv_obj_t* mode_label = NULL;
static lv_obj_t* content_area = NULL;
static lv_obj_t* mode_screens[MODE_MAX] = {0};

static void create_common_ui_elements(lv_obj_t* parent)
{
    // 创建顶部状态栏
    lv_obj_t* status_bar = lv_obj_create(parent);
    lv_obj_set_size(status_bar, LV_PCT(100), 40);
    lv_obj_set_align(status_bar, LV_ALIGN_TOP_MID);
    lv_obj_set_style_bg_color(status_bar, lv_color_hex(0x2C2C2C), 0);
    lv_obj_set_style_border_width(status_bar, 0, 0);
    lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);
    
    // 模式显示标签
    mode_label = lv_label_create(status_bar);
    lv_obj_set_style_text_color(mode_label, lv_color_white(), 0);
    lv_label_set_text(mode_label, "模式显示");
    lv_obj_center(mode_label);
    
    // 创建内容区域
    content_area = lv_obj_create(parent);
    lv_obj_set_size(content_area, LV_PCT(100), LV_PCT(100));
    lv_obj_set_align(content_area, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_y(content_area, -40);
    lv_obj_set_style_border_width(content_area, 0, 0);
    lv_obj_set_style_bg_color(content_area, lv_color_hex(0x000000), 0);
}

static void create_network_config_ui(void)
{
    lv_obj_t* screen = lv_obj_create(NULL);
    mode_screens[MODE_NETWORK_CONFIG] = screen;
    
    create_common_ui_elements(screen);
    lv_label_set_text(mode_label, "📶 配网模式");
    
    // 网络配置界面内容
    lv_obj_t* label = lv_label_create(content_area);
    lv_label_set_text(label, "配网功能准备中...\n\n"
                            "• 短按按键切换模式\n"
                            "• 长按按键进入配网");
    lv_obj_set_style_text_color(label, lv_color_hex(0x00FF00), 0);
    lv_obj_center(label);
}

static void create_clock_ui(void)
{
    lv_obj_t* screen = lv_obj_create(NULL);
    mode_screens[MODE_CLOCK] = screen;
    
    create_common_ui_elements(screen);
    lv_label_set_text(mode_label, "🕒 时钟模式");
    
    // 时钟界面内容 - 使用基本字体
    lv_obj_t* time_label = lv_label_create(content_area);
    lv_label_set_text(time_label, "12:00:00");
    lv_obj_set_style_text_color(time_label, lv_color_hex(0xFFD700), 0);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_14, 0); // 使用14号字体
    lv_obj_center(time_label);
    
    lv_obj_t* date_label = lv_label_create(content_area);
    lv_label_set_text(date_label, "2024年1月1日 星期一");
    lv_obj_set_style_text_color(date_label, lv_color_hex(0x87CEEB), 0);
    lv_obj_align(date_label, LV_ALIGN_CENTER, 0, 60);
}

static void create_weather_ui(void)
{
    lv_obj_t* screen = lv_obj_create(NULL);
    mode_screens[MODE_WEATHER] = screen;
    
    create_common_ui_elements(screen);
    lv_label_set_text(mode_label, "☀️ 天气模式");
    
    // 天气界面内容 - 使用基本字体
    lv_obj_t* weather_icon = lv_label_create(content_area);
    lv_label_set_text(weather_icon, "☀️");
    lv_obj_align(weather_icon, LV_ALIGN_CENTER, 0, -40);
    
    lv_obj_t* temp_label = lv_label_create(content_area);
    lv_label_set_text(temp_label, "25°C");
    lv_obj_set_style_text_color(temp_label, lv_color_hex(0xFF4500), 0);
    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_14, 0); // 使用14号字体
    lv_obj_align(temp_label, LV_ALIGN_CENTER, 0, 20);
    
    lv_obj_t* city_label = lv_label_create(content_area);
    lv_label_set_text(city_label, "北京市");
    lv_obj_set_style_text_color(city_label, lv_color_hex(0x87CEEB), 0);
    lv_obj_align(city_label, LV_ALIGN_CENTER, 0, 70);
}

static void create_gallery_ui(void)
{
    lv_obj_t* screen = lv_obj_create(NULL);
    mode_screens[MODE_GALLERY] = screen;
    
    create_common_ui_elements(screen);
    lv_label_set_text(mode_label, "🖼️ 相册模式");
    
    // 相册界面内容
    lv_obj_t* label = lv_label_create(content_area);
    lv_label_set_text(label, "相册功能准备中...\n\n"
                            "• 左右滑动切换图片\n"
                            "• 支持多格式图片显示");
    lv_obj_set_style_text_color(label, lv_color_hex(0xFF69B4), 0);
    lv_obj_center(label);
}

static void create_virtual_keyboard_ui(void)
{
    lv_obj_t* screen = lv_obj_create(NULL);
    mode_screens[MODE_VIRTUAL_KEYBOARD] = screen;
    
    create_common_ui_elements(screen);
    lv_label_set_text(mode_label, "⌨️ 虚拟键盘模式");
    
    // 虚拟键盘界面内容
    lv_obj_t* label = lv_label_create(content_area);
    lv_label_set_text(label, "虚拟键盘准备中...\n\n"
                            "• 自定义触摸区域\n"
                            "• 远程控制PC操作");
    lv_obj_set_style_text_color(label, lv_color_hex(0x9370DB), 0);
    lv_obj_center(label);
}

void ui_manager_init(lv_display_t* display)
{
    main_display = display;
    
    // 创建所有模式的UI界面
    create_network_config_ui();
    create_clock_ui();
    create_weather_ui();
    create_gallery_ui();
    create_virtual_keyboard_ui();
    
    ESP_LOGI(TAG, "UI manager initialized with %d modes", MODE_MAX);
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

// 各个模式的具体UI创建函数
void ui_create_network_config_screen(void) { /* 已实现 */ }
void ui_create_clock_screen(void) { /* 已实现 */ }
void ui_create_weather_screen(void) { /* 已实现 */ }
void ui_create_gallery_screen(void) { /* 已实现 */ }
void ui_create_virtual_keyboard_screen(void) { /* 已实现 */ }