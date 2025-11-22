#pragma once
#include <stdint.h>

void lcd_touch_init(void);
void lcd_fill_color(uint16_t color);
void lcd_print_mode(const char *name);

// 新增：获取LCD面板句柄（供ui_port.c使用）
void* lcd_get_panel_handle(void);