#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "user_config.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// UI管理器初始化
void ui_manager_init(lv_display_t* display);

// 更新UI显示当前模式
void ui_update_current_mode(device_mode_t mode);

// 创建网络配置界面
void ui_create_network_config_screen(void);

// 创建时钟界面  
void ui_create_clock_screen(void);

// 创建天气界面
void ui_create_weather_screen(void);

// 创建相册界面
void ui_create_gallery_screen(void);

// 创建虚拟键盘界面
void ui_create_virtual_keyboard_screen(void);

#ifdef __cplusplus
}
#endif

#endif