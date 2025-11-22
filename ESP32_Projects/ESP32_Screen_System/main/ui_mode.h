#pragma once
#include "lvgl.h"
#include "key_driver.h"  // 使用统一的sys_mode_t

// UI初始化
void ui_init(void);

// 创建各个模式的屏幕
void ui_create_netcfg_screen(void);
void ui_create_clock_screen(void);
void ui_create_weather_screen(void);
void ui_create_gallery_screen(void);
void ui_create_vkeyboard_screen(void);

// 切换模式
void ui_switch_mode(sys_mode_t mode);