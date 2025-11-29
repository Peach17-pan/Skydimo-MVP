/**
 * @file ui_screens.c
 * @brief UI 界面管理模块实现
 */

#include "ui_screens.h"
#include "user_config.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "ui_screens";

/* UI 管理器单例 */
static ui_manager_t ui_mgr = {0};

/* 颜色定义 - 现代深色主题 */
#define COLOR_BG_DARK       lv_color_hex(0x1A1A2E)
#define COLOR_BG_CARD       lv_color_hex(0x16213E)
#define COLOR_PRIMARY       lv_color_hex(0x0F3460)
#define COLOR_ACCENT        lv_color_hex(0xE94560)
#define COLOR_TEXT_MAIN     lv_color_hex(0xEEEEEE)
#define COLOR_TEXT_SEC      lv_color_hex(0x94A3B8)
#define COLOR_SUCCESS       lv_color_hex(0x10B981)
#define COLOR_WARNING       lv_color_hex(0xF59E0B)

/* 样式 */
static lv_style_t style_card;
static lv_style_t style_title;
static lv_style_t style_text;
static lv_style_t style_text_big;

/******************** 静态函数声明 ********************/
static void init_styles(void);
static void create_mode_indicator(lv_obj_t *parent);
static void create_wifi_panel(lv_obj_t *parent);
static void create_clock_panel(lv_obj_t *parent);
static void create_weather_panel(lv_obj_t *parent);
static void create_photo_panel(lv_obj_t *parent);
static void create_keyboard_panel(lv_obj_t *parent);
static void hide_all_panels(void);
static void keyboard_event_cb(lv_event_t *e);

/******************** 样式初始化 ********************/
static void init_styles(void)
{
    /* 卡片样式 */
    lv_style_init(&style_card);
    lv_style_set_bg_color(&style_card, COLOR_BG_CARD);
    lv_style_set_bg_opa(&style_card, LV_OPA_COVER);
    lv_style_set_radius(&style_card, 12);
    lv_style_set_pad_all(&style_card, 15);
    lv_style_set_border_width(&style_card, 1);
    lv_style_set_border_color(&style_card, COLOR_PRIMARY);
    
    /* 标题样式 */
    lv_style_init(&style_title);
    lv_style_set_text_color(&style_title, COLOR_ACCENT);
    lv_style_set_text_font(&style_title, &lv_font_montserrat_14);
    
    /* 正文样式 */
    lv_style_init(&style_text);
    lv_style_set_text_color(&style_text, COLOR_TEXT_MAIN);
    lv_style_set_text_font(&style_text, &lv_font_montserrat_14);
    
    /* 大字体样式 */
    lv_style_init(&style_text_big);
    lv_style_set_text_color(&style_text_big, COLOR_TEXT_MAIN);
    lv_style_set_text_font(&style_text_big, &lv_font_montserrat_14);
}

/******************** 模式指示器 ********************/
static void create_mode_indicator(lv_obj_t *parent)
{
    /* 顶部模式标签容器 */
    lv_obj_t *header = lv_obj_create(parent);
    lv_obj_set_size(header, EXAMPLE_LCD_H_RES - 10, 40);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_bg_color(header, COLOR_PRIMARY, 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(header, 8, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 模式标签 */
    ui_mgr.mode_label = lv_label_create(header);
    lv_obj_add_style(ui_mgr.mode_label, &style_title, 0);
    lv_label_set_text(ui_mgr.mode_label, "时钟");
    lv_obj_center(ui_mgr.mode_label);
}

/******************** WiFi 配网界面 ********************/
static void create_wifi_panel(lv_obj_t *parent)
{
    ui_mgr.wifi_panel = lv_obj_create(parent);
    lv_obj_set_size(ui_mgr.wifi_panel, EXAMPLE_LCD_H_RES - 10, EXAMPLE_LCD_V_RES - 60);
    lv_obj_align(ui_mgr.wifi_panel, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_add_style(ui_mgr.wifi_panel, &style_card, 0);
    lv_obj_remove_flag(ui_mgr.wifi_panel, LV_OBJ_FLAG_SCROLLABLE);
    
    /* WiFi 图标区域 */
    lv_obj_t *icon_cont = lv_obj_create(ui_mgr.wifi_panel);
    lv_obj_set_size(icon_cont, 80, 80);
    lv_obj_align(icon_cont, LV_ALIGN_TOP_MID, 0, 15);
    lv_obj_set_style_bg_color(icon_cont, COLOR_PRIMARY, 0);
    lv_obj_set_style_bg_opa(icon_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(icon_cont, 40, 0);
    lv_obj_set_style_border_width(icon_cont, 2, 0);
    lv_obj_set_style_border_color(icon_cont, COLOR_ACCENT, 0);
    
    lv_obj_t *icon_label = lv_label_create(icon_cont);
    lv_label_set_text(icon_label, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(icon_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(icon_label, COLOR_ACCENT, 0);
    lv_obj_center(icon_label);
    
    /* 标题 */
    lv_obj_t *title = lv_label_create(ui_mgr.wifi_panel);
    lv_obj_add_style(title, &style_title, 0);
    lv_label_set_text(title, "WiFi Provision Mode");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 105);
    
    /* 状态显示 */
    ui_mgr.wifi_status_label = lv_label_create(ui_mgr.wifi_panel);
    lv_obj_add_style(ui_mgr.wifi_status_label, &style_text, 0);
    lv_label_set_text(ui_mgr.wifi_status_label, "Waiting for connection...");
    lv_obj_set_style_text_color(ui_mgr.wifi_status_label, COLOR_WARNING, 0);
    lv_obj_align(ui_mgr.wifi_status_label, LV_ALIGN_TOP_MID, 0, 140);
    
    /* 分隔线 */
    lv_obj_t *sep_line = lv_obj_create(ui_mgr.wifi_panel);
    lv_obj_set_size(sep_line, 120, 2);
    lv_obj_align(sep_line, LV_ALIGN_TOP_MID, 0, 175);
    lv_obj_set_style_bg_color(sep_line, COLOR_PRIMARY, 0);
    lv_obj_set_style_bg_opa(sep_line, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep_line, 0, 0);
    
    /* AP 信息区域 */
    lv_obj_t *ap_info_cont = lv_obj_create(ui_mgr.wifi_panel);
    lv_obj_set_size(ap_info_cont, EXAMPLE_LCD_H_RES - 35, 130);
    lv_obj_align(ap_info_cont, LV_ALIGN_TOP_MID, 0, 190);
    lv_obj_set_style_bg_color(ap_info_cont, COLOR_PRIMARY, 0);
    lv_obj_set_style_bg_opa(ap_info_cont, LV_OPA_50, 0);
    lv_obj_set_style_radius(ap_info_cont, 8, 0);
    lv_obj_set_style_border_width(ap_info_cont, 0, 0);
    lv_obj_set_style_pad_all(ap_info_cont, 10, 0);
    lv_obj_remove_flag(ap_info_cont, LV_OBJ_FLAG_SCROLLABLE);
    
    /* AP 信息标题 */
    lv_obj_t *ap_title = lv_label_create(ap_info_cont);
    lv_obj_add_style(ap_title, &style_text, 0);
    lv_label_set_text(ap_title, "Connect to this WiFi:");
    lv_obj_set_style_text_color(ap_title, COLOR_TEXT_SEC, 0);
    lv_obj_align(ap_title, LV_ALIGN_TOP_MID, 0, 5);
    
    /* AP SSID 显示 */
    ui_mgr.wifi_ap_ssid_label = lv_label_create(ap_info_cont);
    lv_obj_add_style(ui_mgr.wifi_ap_ssid_label, &style_text, 0);
    lv_label_set_text(ui_mgr.wifi_ap_ssid_label, "SSID: Skydimo_Config");
    lv_obj_set_style_text_color(ui_mgr.wifi_ap_ssid_label, COLOR_SUCCESS, 0);
    lv_obj_align(ui_mgr.wifi_ap_ssid_label, LV_ALIGN_TOP_MID, 0, 35);
    
    /* AP 密码显示 */
    lv_obj_t *ap_pass = lv_label_create(ap_info_cont);
    lv_obj_add_style(ap_pass, &style_text, 0);
    lv_label_set_text(ap_pass, "Pass: 12345678");
    lv_obj_align(ap_pass, LV_ALIGN_TOP_MID, 0, 60);
    
    /* AP IP 地址显示 */
    ui_mgr.wifi_ap_ip_label = lv_label_create(ap_info_cont);
    lv_obj_add_style(ui_mgr.wifi_ap_ip_label, &style_text, 0);
    lv_label_set_text(ui_mgr.wifi_ap_ip_label, "IP: 192.168.4.1");
    lv_obj_set_style_text_color(ui_mgr.wifi_ap_ip_label, COLOR_ACCENT, 0);
    lv_obj_align(ui_mgr.wifi_ap_ip_label, LV_ALIGN_TOP_MID, 0, 85);
    
    /* 操作说明区域 */
    lv_obj_t *help_cont = lv_obj_create(ui_mgr.wifi_panel);
    lv_obj_set_size(help_cont, EXAMPLE_LCD_H_RES - 35, 170);
    lv_obj_align(help_cont, LV_ALIGN_TOP_MID, 0, 335);
    lv_obj_set_style_bg_color(help_cont, COLOR_PRIMARY, 0);
    lv_obj_set_style_bg_opa(help_cont, LV_OPA_30, 0);
    lv_obj_set_style_radius(help_cont, 8, 0);
    lv_obj_set_style_border_width(help_cont, 0, 0);
    lv_obj_set_style_pad_all(help_cont, 10, 0);
    lv_obj_remove_flag(help_cont, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 操作步骤 */
    lv_obj_t *step1 = lv_label_create(help_cont);
    lv_obj_add_style(step1, &style_text, 0);
    lv_label_set_text(step1, "1. Connect phone to\n   'Skydimo_Config'");
    lv_obj_set_style_text_color(step1, COLOR_TEXT_SEC, 0);
    lv_obj_align(step1, LV_ALIGN_TOP_LEFT, 5, 5);
    
    lv_obj_t *step2 = lv_label_create(help_cont);
    lv_obj_add_style(step2, &style_text, 0);
    lv_label_set_text(step2, "2. Open browser and\n   visit 192.168.4.1");
    lv_obj_set_style_text_color(step2, COLOR_TEXT_SEC, 0);
    lv_obj_align(step2, LV_ALIGN_TOP_LEFT, 5, 55);
    
    lv_obj_t *step3 = lv_label_create(help_cont);
    lv_obj_add_style(step3, &style_text, 0);
    lv_label_set_text(step3, "3. Enter your WiFi\n   credentials");
    lv_obj_set_style_text_color(step3, COLOR_TEXT_SEC, 0);
    lv_obj_align(step3, LV_ALIGN_TOP_LEFT, 5, 105);
    
    /* 提示信息 - 可更新 */
    ui_mgr.wifi_hint_label = lv_label_create(ui_mgr.wifi_panel);
    lv_obj_add_style(ui_mgr.wifi_hint_label, &style_text, 0);
    lv_label_set_text(ui_mgr.wifi_hint_label, "Long press to exit");
    lv_obj_set_style_text_align(ui_mgr.wifi_hint_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(ui_mgr.wifi_hint_label, COLOR_ACCENT, 0);
    lv_obj_align(ui_mgr.wifi_hint_label, LV_ALIGN_BOTTOM_MID, 0, -15);
    
    lv_obj_add_flag(ui_mgr.wifi_panel, LV_OBJ_FLAG_HIDDEN);
}

/******************** 时钟界面 ********************/
static void create_clock_panel(lv_obj_t *parent)
{
    ui_mgr.clock_panel = lv_obj_create(parent);
    lv_obj_set_size(ui_mgr.clock_panel, EXAMPLE_LCD_H_RES - 10, EXAMPLE_LCD_V_RES - 60);
    lv_obj_align(ui_mgr.clock_panel, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_add_style(ui_mgr.clock_panel, &style_card, 0);
    lv_obj_remove_flag(ui_mgr.clock_panel, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 装饰性圆环 */
    lv_obj_t *ring = lv_obj_create(ui_mgr.clock_panel);
    lv_obj_set_size(ring, 130, 130);
    lv_obj_align(ring, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_set_style_bg_opa(ring, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ring, 3, 0);
    lv_obj_set_style_border_color(ring, COLOR_ACCENT, 0);
    lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_remove_flag(ring, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 时间显示 */
    ui_mgr.clock_time_label = lv_label_create(ring);
    lv_obj_add_style(ui_mgr.clock_time_label, &style_text_big, 0);
    lv_label_set_text(ui_mgr.clock_time_label, "12:00:00");
    lv_obj_center(ui_mgr.clock_time_label);
    
    /* 日期显示 */
    ui_mgr.clock_date_label = lv_label_create(ui_mgr.clock_panel);
    lv_obj_add_style(ui_mgr.clock_date_label, &style_text, 0);
    lv_label_set_text(ui_mgr.clock_date_label, "2025-01-01");
    lv_obj_set_style_text_color(ui_mgr.clock_date_label, COLOR_TEXT_SEC, 0);
    lv_obj_align(ui_mgr.clock_date_label, LV_ALIGN_TOP_MID, 0, 180);
    
    /* 星期显示 */
    lv_obj_t *weekday = lv_label_create(ui_mgr.clock_panel);
    lv_obj_add_style(weekday, &style_text, 0);
    lv_label_set_text(weekday, "Thursday");
    lv_obj_set_style_text_color(weekday, COLOR_ACCENT, 0);
    lv_obj_align(weekday, LV_ALIGN_TOP_MID, 0, 210);
    
    /* 分隔线 */
    lv_obj_t *line = lv_obj_create(ui_mgr.clock_panel);
    lv_obj_set_size(line, 100, 2);
    lv_obj_align(line, LV_ALIGN_TOP_MID, 0, 250);
    lv_obj_set_style_bg_color(line, COLOR_PRIMARY, 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(line, 0, 0);
    
    /* 信息区 */
    lv_obj_t *info_cont = lv_obj_create(ui_mgr.clock_panel);
    lv_obj_set_size(info_cont, EXAMPLE_LCD_H_RES - 40, 150);
    lv_obj_align(info_cont, LV_ALIGN_TOP_MID, 0, 270);
    lv_obj_set_style_bg_color(info_cont, COLOR_PRIMARY, 0);
    lv_obj_set_style_bg_opa(info_cont, LV_OPA_50, 0);
    lv_obj_set_style_radius(info_cont, 8, 0);
    lv_obj_set_style_border_width(info_cont, 0, 0);
    lv_obj_remove_flag(info_cont, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 闹钟图标和时间 */
    lv_obj_t *alarm_icon = lv_label_create(info_cont);
    lv_label_set_text(alarm_icon, LV_SYMBOL_BELL);
    lv_obj_set_style_text_color(alarm_icon, COLOR_WARNING, 0);
    lv_obj_align(alarm_icon, LV_ALIGN_TOP_LEFT, 10, 15);
    
    lv_obj_t *alarm_text = lv_label_create(info_cont);
    lv_obj_add_style(alarm_text, &style_text, 0);
    lv_label_set_text(alarm_text, "No alarm set");
    lv_obj_align(alarm_text, LV_ALIGN_TOP_LEFT, 35, 15);
    
    /* 秒表 */
    lv_obj_t *timer_icon = lv_label_create(info_cont);
    lv_label_set_text(timer_icon, LV_SYMBOL_PAUSE);
    lv_obj_set_style_text_color(timer_icon, COLOR_SUCCESS, 0);
    lv_obj_align(timer_icon, LV_ALIGN_TOP_LEFT, 10, 50);
    
    lv_obj_t *timer_text = lv_label_create(info_cont);
    lv_obj_add_style(timer_text, &style_text, 0);
    lv_label_set_text(timer_text, "Stopwatch: 00:00");
    lv_obj_align(timer_text, LV_ALIGN_TOP_LEFT, 35, 50);
    
    /* 按键提示 */
    lv_obj_t *hint = lv_label_create(info_cont);
    lv_obj_add_style(hint, &style_text, 0);
    lv_label_set_text(hint, "BOOT: Next Mode");
    lv_obj_set_style_text_color(hint, COLOR_TEXT_SEC, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    lv_obj_add_flag(ui_mgr.clock_panel, LV_OBJ_FLAG_HIDDEN);
}

/******************** 天气界面 ********************/
static void create_weather_panel(lv_obj_t *parent)
{
    ui_mgr.weather_panel = lv_obj_create(parent);
    lv_obj_set_size(ui_mgr.weather_panel, EXAMPLE_LCD_H_RES - 10, EXAMPLE_LCD_V_RES - 60);
    lv_obj_align(ui_mgr.weather_panel, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_add_style(ui_mgr.weather_panel, &style_card, 0);
    lv_obj_remove_flag(ui_mgr.weather_panel, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 天气图标区域 */
    lv_obj_t *icon_area = lv_obj_create(ui_mgr.weather_panel);
    lv_obj_set_size(icon_area, 100, 100);
    lv_obj_align(icon_area, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_bg_color(icon_area, COLOR_PRIMARY, 0);
    lv_obj_set_style_bg_opa(icon_area, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(icon_area, 50, 0);
    lv_obj_set_style_border_width(icon_area, 2, 0);
    lv_obj_set_style_border_color(icon_area, COLOR_WARNING, 0);
    lv_obj_remove_flag(icon_area, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 天气图标（使用文字模拟） */
    ui_mgr.weather_icon = lv_label_create(icon_area);
    lv_label_set_text(ui_mgr.weather_icon, "SUN");
    lv_obj_set_style_text_font(ui_mgr.weather_icon, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ui_mgr.weather_icon, COLOR_WARNING, 0);
    lv_obj_center(ui_mgr.weather_icon);
    
    /* 温度显示 */
    ui_mgr.weather_temp_label = lv_label_create(ui_mgr.weather_panel);
    lv_obj_add_style(ui_mgr.weather_temp_label, &style_text_big, 0);
    lv_label_set_text(ui_mgr.weather_temp_label, "25°C");
    lv_obj_set_style_text_color(ui_mgr.weather_temp_label, COLOR_WARNING, 0);
    lv_obj_align(ui_mgr.weather_temp_label, LV_ALIGN_TOP_MID, 0, 135);
    
    /* 天气描述 */
    ui_mgr.weather_desc_label = lv_label_create(ui_mgr.weather_panel);
    lv_obj_add_style(ui_mgr.weather_desc_label, &style_text, 0);
    lv_label_set_text(ui_mgr.weather_desc_label, "Sunny");
    lv_obj_align(ui_mgr.weather_desc_label, LV_ALIGN_TOP_MID, 0, 165);
    
    /* 位置 */
    lv_obj_t *location = lv_label_create(ui_mgr.weather_panel);
    lv_obj_add_style(location, &style_text, 0);
    lv_label_set_text(location, LV_SYMBOL_GPS " Shenzhen");
    lv_obj_set_style_text_color(location, COLOR_TEXT_SEC, 0);
    lv_obj_align(location, LV_ALIGN_TOP_MID, 0, 195);
    
    /* 详细信息区 */
    lv_obj_t *detail_cont = lv_obj_create(ui_mgr.weather_panel);
    lv_obj_set_size(detail_cont, EXAMPLE_LCD_H_RES - 40, 180);
    lv_obj_align(detail_cont, LV_ALIGN_TOP_MID, 0, 230);
    lv_obj_set_style_bg_color(detail_cont, COLOR_PRIMARY, 0);
    lv_obj_set_style_bg_opa(detail_cont, LV_OPA_50, 0);
    lv_obj_set_style_radius(detail_cont, 8, 0);
    lv_obj_set_style_border_width(detail_cont, 0, 0);
    lv_obj_set_style_pad_all(detail_cont, 10, 0);
    lv_obj_remove_flag(detail_cont, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 湿度 */
    lv_obj_t *humid_label = lv_label_create(detail_cont);
    lv_obj_add_style(humid_label, &style_text, 0);
    lv_label_set_text(humid_label, LV_SYMBOL_CHARGE " Humidity: 65%");
    lv_obj_align(humid_label, LV_ALIGN_TOP_LEFT, 5, 10);
    
    /* 风速 */
    lv_obj_t *wind_label = lv_label_create(detail_cont);
    lv_obj_add_style(wind_label, &style_text, 0);
    lv_label_set_text(wind_label, LV_SYMBOL_LOOP " Wind: 12 km/h");
    lv_obj_align(wind_label, LV_ALIGN_TOP_LEFT, 5, 40);
    
    /* 紫外线 */
    lv_obj_t *uv_label = lv_label_create(detail_cont);
    lv_obj_add_style(uv_label, &style_text, 0);
    lv_label_set_text(uv_label, LV_SYMBOL_EYE_OPEN " UV Index: 3");
    lv_obj_align(uv_label, LV_ALIGN_TOP_LEFT, 5, 70);
    
    /* 空气质量 */
    lv_obj_t *aqi_label = lv_label_create(detail_cont);
    lv_obj_add_style(aqi_label, &style_text, 0);
    lv_label_set_text(aqi_label, LV_SYMBOL_OK " AQI: Good");
    lv_obj_set_style_text_color(aqi_label, COLOR_SUCCESS, 0);
    lv_obj_align(aqi_label, LV_ALIGN_TOP_LEFT, 5, 100);
    
    /* 按键提示 */
    lv_obj_t *hint = lv_label_create(detail_cont);
    lv_obj_add_style(hint, &style_text, 0);
    lv_label_set_text(hint, "BOOT: Next Mode");
    lv_obj_set_style_text_color(hint, COLOR_TEXT_SEC, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -5);
    
    lv_obj_add_flag(ui_mgr.weather_panel, LV_OBJ_FLAG_HIDDEN);
}

/******************** 相册界面 ********************/
static void create_photo_panel(lv_obj_t *parent)
{
    ui_mgr.photo_panel = lv_obj_create(parent);
    lv_obj_set_size(ui_mgr.photo_panel, EXAMPLE_LCD_H_RES - 10, EXAMPLE_LCD_V_RES - 60);
    lv_obj_align(ui_mgr.photo_panel, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_add_style(ui_mgr.photo_panel, &style_card, 0);
    lv_obj_remove_flag(ui_mgr.photo_panel, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 相框 */
    lv_obj_t *frame = lv_obj_create(ui_mgr.photo_panel);
    lv_obj_set_size(frame, EXAMPLE_LCD_H_RES - 40, 320);
    lv_obj_align(frame, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_bg_color(frame, COLOR_PRIMARY, 0);
    lv_obj_set_style_bg_opa(frame, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(frame, 8, 0);
    lv_obj_set_style_border_width(frame, 3, 0);
    lv_obj_set_style_border_color(frame, COLOR_ACCENT, 0);
    lv_obj_remove_flag(frame, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 图片占位符 */
    ui_mgr.photo_image = lv_label_create(frame);
    lv_label_set_text(ui_mgr.photo_image, LV_SYMBOL_IMAGE);
    lv_obj_set_style_text_font(ui_mgr.photo_image, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ui_mgr.photo_image, COLOR_TEXT_SEC, 0);
    lv_obj_center(ui_mgr.photo_image);
    
    /* 无图片提示 */
    lv_obj_t *no_photo = lv_label_create(frame);
    lv_obj_add_style(no_photo, &style_text, 0);
    lv_label_set_text(no_photo, "No photos in SD");
    lv_obj_set_style_text_color(no_photo, COLOR_TEXT_SEC, 0);
    lv_obj_align(no_photo, LV_ALIGN_CENTER, 0, 30);
    
    /* 图片信息 */
    ui_mgr.photo_info_label = lv_label_create(ui_mgr.photo_panel);
    lv_obj_add_style(ui_mgr.photo_info_label, &style_text, 0);
    lv_label_set_text(ui_mgr.photo_info_label, "Photo: 0 / 0");
    lv_obj_set_style_text_color(ui_mgr.photo_info_label, COLOR_TEXT_SEC, 0);
    lv_obj_align(ui_mgr.photo_info_label, LV_ALIGN_TOP_MID, 0, 355);
    
    /* 控制区 */
    lv_obj_t *ctrl_cont = lv_obj_create(ui_mgr.photo_panel);
    lv_obj_set_size(ctrl_cont, EXAMPLE_LCD_H_RES - 40, 100);
    lv_obj_align(ctrl_cont, LV_ALIGN_TOP_MID, 0, 385);
    lv_obj_set_style_bg_color(ctrl_cont, COLOR_PRIMARY, 0);
    lv_obj_set_style_bg_opa(ctrl_cont, LV_OPA_50, 0);
    lv_obj_set_style_radius(ctrl_cont, 8, 0);
    lv_obj_set_style_border_width(ctrl_cont, 0, 0);
    lv_obj_remove_flag(ctrl_cont, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 上一张按钮标签 */
    lv_obj_t *prev_label = lv_label_create(ctrl_cont);
    lv_obj_add_style(prev_label, &style_text, 0);
    lv_label_set_text(prev_label, LV_SYMBOL_LEFT " PWR: Prev");
    lv_obj_align(prev_label, LV_ALIGN_TOP_LEFT, 10, 15);
    
    /* 下一张按钮标签 */
    lv_obj_t *next_label = lv_label_create(ctrl_cont);
    lv_obj_add_style(next_label, &style_text, 0);
    lv_label_set_text(next_label, "BOOT: Next " LV_SYMBOL_RIGHT);
    lv_obj_align(next_label, LV_ALIGN_TOP_LEFT, 10, 45);
    
    /* 按键提示 */
    lv_obj_t *hint = lv_label_create(ctrl_cont);
    lv_obj_add_style(hint, &style_text, 0);
    lv_label_set_text(hint, "Long: Switch Mode");
    lv_obj_set_style_text_color(hint, COLOR_TEXT_SEC, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    lv_obj_add_flag(ui_mgr.photo_panel, LV_OBJ_FLAG_HIDDEN);
}

/******************** 虚拟键盘界面 ********************/
static void keyboard_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *kb = (lv_obj_t *)lv_event_get_target(e);
    
    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        /* 键盘确认或取消 */
        ESP_LOGI(TAG, "Keyboard: %s", code == LV_EVENT_READY ? "Ready" : "Cancel");
    }
}

static void create_keyboard_panel(lv_obj_t *parent)
{
    ui_mgr.keyboard_panel = lv_obj_create(parent);
    lv_obj_set_size(ui_mgr.keyboard_panel, EXAMPLE_LCD_H_RES - 10, EXAMPLE_LCD_V_RES - 60);
    lv_obj_align(ui_mgr.keyboard_panel, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_add_style(ui_mgr.keyboard_panel, &style_card, 0);
    lv_obj_remove_flag(ui_mgr.keyboard_panel, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 标题 */
    lv_obj_t *title = lv_label_create(ui_mgr.keyboard_panel);
    lv_obj_add_style(title, &style_title, 0);
    lv_label_set_text(title, "Virtual Keyboard");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    /* 文本输入区 */
    ui_mgr.keyboard_ta = lv_textarea_create(ui_mgr.keyboard_panel);
    lv_obj_set_size(ui_mgr.keyboard_ta, EXAMPLE_LCD_H_RES - 40, 100);
    lv_obj_align(ui_mgr.keyboard_ta, LV_ALIGN_TOP_MID, 0, 45);
    lv_textarea_set_placeholder_text(ui_mgr.keyboard_ta, "Type here...");
    lv_obj_set_style_bg_color(ui_mgr.keyboard_ta, COLOR_PRIMARY, 0);
    lv_obj_set_style_text_color(ui_mgr.keyboard_ta, COLOR_TEXT_MAIN, 0);
    lv_obj_set_style_border_color(ui_mgr.keyboard_ta, COLOR_ACCENT, LV_STATE_FOCUSED);
    
    /* 键盘 */
    ui_mgr.keyboard_obj = lv_keyboard_create(ui_mgr.keyboard_panel);
    lv_obj_set_size(ui_mgr.keyboard_obj, EXAMPLE_LCD_H_RES - 30, 350);
    lv_obj_align(ui_mgr.keyboard_obj, LV_ALIGN_TOP_MID, 0, 160);
    lv_keyboard_set_textarea(ui_mgr.keyboard_obj, ui_mgr.keyboard_ta);
    lv_obj_add_event_cb(ui_mgr.keyboard_obj, keyboard_event_cb, LV_EVENT_ALL, NULL);
    
    /* 键盘样式 */
    lv_obj_set_style_bg_color(ui_mgr.keyboard_obj, COLOR_BG_CARD, 0);
    lv_obj_set_style_bg_color(ui_mgr.keyboard_obj, COLOR_PRIMARY, LV_PART_ITEMS);
    lv_obj_set_style_text_color(ui_mgr.keyboard_obj, COLOR_TEXT_MAIN, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(ui_mgr.keyboard_obj, COLOR_ACCENT, LV_PART_ITEMS | LV_STATE_PRESSED);
    
    /* 按键提示 */
    lv_obj_t *hint = lv_label_create(ui_mgr.keyboard_panel);
    lv_obj_add_style(hint, &style_text, 0);
    lv_label_set_text(hint, "BOOT: Next Mode");
    lv_obj_set_style_text_color(hint, COLOR_TEXT_SEC, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -15);
    
    lv_obj_add_flag(ui_mgr.keyboard_panel, LV_OBJ_FLAG_HIDDEN);
}

/******************** 隐藏所有面板 ********************/
static void hide_all_panels(void)
{
    if (ui_mgr.wifi_panel) lv_obj_add_flag(ui_mgr.wifi_panel, LV_OBJ_FLAG_HIDDEN);
    if (ui_mgr.clock_panel) lv_obj_add_flag(ui_mgr.clock_panel, LV_OBJ_FLAG_HIDDEN);
    if (ui_mgr.weather_panel) lv_obj_add_flag(ui_mgr.weather_panel, LV_OBJ_FLAG_HIDDEN);
    if (ui_mgr.photo_panel) lv_obj_add_flag(ui_mgr.photo_panel, LV_OBJ_FLAG_HIDDEN);
    if (ui_mgr.keyboard_panel) lv_obj_add_flag(ui_mgr.keyboard_panel, LV_OBJ_FLAG_HIDDEN);
}

/******************** 公共函数实现 ********************/
ui_manager_t* ui_screens_init(void)
{
    ESP_LOGI(TAG, "Initializing UI screens...");
    
    /* 初始化样式 */
    init_styles();
    
    /* 获取当前屏幕 */
    lv_obj_t *scr = lv_screen_active();
    
    /* 设置背景色 */
    lv_obj_set_style_bg_color(scr, COLOR_BG_DARK, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    
    /* 创建主容器 */
    ui_mgr.main_container = lv_obj_create(scr);
    lv_obj_set_size(ui_mgr.main_container, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    lv_obj_align(ui_mgr.main_container, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(ui_mgr.main_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_mgr.main_container, 0, 0);
    lv_obj_set_style_pad_all(ui_mgr.main_container, 0, 0);
    lv_obj_remove_flag(ui_mgr.main_container, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 创建模式指示器 */
    create_mode_indicator(ui_mgr.main_container);
    
    /* 创建各模式面板 */
    create_wifi_panel(ui_mgr.main_container);
    create_clock_panel(ui_mgr.main_container);
    create_weather_panel(ui_mgr.main_container);
    create_photo_panel(ui_mgr.main_container);
    create_keyboard_panel(ui_mgr.main_container);
    
    /* 默认显示时钟界面 */
    ui_screens_switch_to(APP_MODE_CLOCK);
    
    ESP_LOGI(TAG, "UI screens initialized successfully");
    
    return &ui_mgr;
}

void ui_screens_switch_to(app_mode_t mode)
{
    /* 隐藏所有面板 */
    hide_all_panels();
    
    /* 更新模式标签 */
    if (ui_mgr.mode_label) {
        lv_label_set_text(ui_mgr.mode_label, app_mode_get_name(mode));
    }
    
    /* 显示对应面板 */
    switch (mode) {
        case APP_MODE_WIFI_CONFIG:
            if (ui_mgr.wifi_panel) lv_obj_remove_flag(ui_mgr.wifi_panel, LV_OBJ_FLAG_HIDDEN);
            break;
        case APP_MODE_CLOCK:
            if (ui_mgr.clock_panel) lv_obj_remove_flag(ui_mgr.clock_panel, LV_OBJ_FLAG_HIDDEN);
            break;
        case APP_MODE_WEATHER:
            if (ui_mgr.weather_panel) lv_obj_remove_flag(ui_mgr.weather_panel, LV_OBJ_FLAG_HIDDEN);
            break;
        case APP_MODE_PHOTO:
            if (ui_mgr.photo_panel) lv_obj_remove_flag(ui_mgr.photo_panel, LV_OBJ_FLAG_HIDDEN);
            break;
        case APP_MODE_KEYBOARD:
            if (ui_mgr.keyboard_panel) lv_obj_remove_flag(ui_mgr.keyboard_panel, LV_OBJ_FLAG_HIDDEN);
            break;
        default:
            break;
    }
    
    ESP_LOGI(TAG, "Switched to mode: %s", app_mode_get_name(mode));
}

void ui_clock_update_time(uint8_t hour, uint8_t minute, uint8_t second)
{
    if (ui_mgr.clock_time_label) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d", hour, minute, second);
        lv_label_set_text(ui_mgr.clock_time_label, buf);
    }
}

void ui_clock_update_date(uint16_t year, uint8_t month, uint8_t day)
{
    if (ui_mgr.clock_date_label) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d", year, month, day);
        lv_label_set_text(ui_mgr.clock_date_label, buf);
    }
}

void ui_weather_update(int temp, const char *desc)
{
    if (ui_mgr.weather_temp_label) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d°C", temp);
        lv_label_set_text(ui_mgr.weather_temp_label, buf);
    }
    if (ui_mgr.weather_desc_label && desc) {
        lv_label_set_text(ui_mgr.weather_desc_label, desc);
    }
}

void ui_wifi_update_status(bool connected, const char *message)
{
    if (ui_mgr.wifi_status_label && message) {
        /* 直接显示状态消息 */
        lv_label_set_text(ui_mgr.wifi_status_label, message);
        lv_obj_set_style_text_color(ui_mgr.wifi_status_label, 
            connected ? COLOR_SUCCESS : COLOR_WARNING, 0);
    }
}

ui_manager_t* ui_get_manager(void)
{
    return &ui_mgr;
}

void ui_wifi_set_provision_mode(bool provision_mode)
{
    if (ui_mgr.wifi_hint_label) {
        if (provision_mode) {
            lv_label_set_text(ui_mgr.wifi_hint_label, "Long press to exit");
        } else {
            lv_label_set_text(ui_mgr.wifi_hint_label, "Long press to enter provision mode");
        }
    }
    
    if (ui_mgr.wifi_status_label) {
        if (provision_mode) {
            lv_label_set_text(ui_mgr.wifi_status_label, "AP Started - Waiting...");
            lv_obj_set_style_text_color(ui_mgr.wifi_status_label, COLOR_WARNING, 0);
        } else {
            lv_label_set_text(ui_mgr.wifi_status_label, "Provision mode inactive");
            lv_obj_set_style_text_color(ui_mgr.wifi_status_label, COLOR_TEXT_SEC, 0);
        }
    }
}

void ui_wifi_update_provision_info(const char *ap_ssid, const char *ap_ip)
{
    if (ui_mgr.wifi_ap_ssid_label && ap_ssid) {
        char buf[48];
        snprintf(buf, sizeof(buf), "SSID: %s", ap_ssid);
        lv_label_set_text(ui_mgr.wifi_ap_ssid_label, buf);
    }
    
    if (ui_mgr.wifi_ap_ip_label && ap_ip) {
        char buf[32];
        snprintf(buf, sizeof(buf), "IP: %s", ap_ip);
        lv_label_set_text(ui_mgr.wifi_ap_ip_label, buf);
    }
}

