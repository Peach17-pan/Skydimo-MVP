#pragma once
#include <stdint.h>

void lcd_touch_init(void);
void lcd_fill_color(uint16_t color);
void lcd_print_mode(const char *name);
void* lcd_get_panel_handle(void);

// 新增函数声明
void lcd_draw_text(int x, int y, const char *text, uint16_t color, uint16_t bg_color);
void lcd_draw_rect(int x, int y, int width, int height, uint16_t color);