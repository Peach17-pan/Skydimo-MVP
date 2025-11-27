// ui_manager.h
#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "../main/system_config.h"

// 初始化UI管理器
esp_err_t ui_manager_init(system_handle_t *sys_handle);

// 显示当前模式
void ui_show_current_mode(system_handle_t *sys_handle);

// 处理触摸事件
void ui_handle_touch(system_handle_t *sys_handle, uint16_t x, uint16_t y);

// 处理长按事件  
void ui_handle_long_press(system_handle_t *sys_handle);

// 获取模式名称
const char* ui_get_mode_name(system_mode_t mode);

// LVGL相关函数
void ui_create_wifi_screen(lv_obj_t *parent);
void ui_create_clock_screen(lv_obj_t *parent);
void ui_create_weather_screen(lv_obj_t *parent);
void ui_create_gallery_screen(lv_obj_t *parent);
void ui_create_keyboard_screen(lv_obj_t *parent);

#endif