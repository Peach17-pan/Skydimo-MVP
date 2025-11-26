#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"



// UI页面枚举
typedef enum {
    UI_PAGE_HOME = 0,
    UI_PAGE_MENU,
    UI_PAGE_SETTINGS,
    UI_PAGE_STATUS
} ui_page_t;

// UI元素结构
typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    const char* text;
    uint32_t bg_color;
    uint32_t text_color;
} ui_element_t;

// 初始化UI管理器
esp_err_t ui_manager_init(system_handle_t *sys_handle);

// 显示当前模式
void ui_show_current_mode(system_handle_t *sys_handle);

// 清除屏幕
void ui_clear_screen(void);

// 绘制居中文本
void ui_draw_text_center(const char *text, uint16_t y, uint16_t color);

// 绘制矩形
void ui_draw_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);

// 处理触摸事件
void ui_handle_touch(system_handle_t *sys_handle, uint16_t x, uint16_t y);

// 处理长按事件
void ui_handle_long_press(system_handle_t *sys_handle);

// 更新状态显示
void ui_update_status_display(system_handle_t *sys_handle);

#endif