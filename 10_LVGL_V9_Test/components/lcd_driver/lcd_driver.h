#ifndef LCD_DRIVER_H
#define LCD_DRIVER_H

#include <stdint.h>
#include <stddef.h>  
#include "stdbool.h"
#include "user_config.h"
void lcd_test_pattern(void);
void lcd_fill_color(uint16_t color);

// 初始化函数
void lcd_driver_init(void);

// 基本通信函数
void lcd_send_data(void *data, size_t len);
void lcd_send_command(uint8_t cmd);
void lcd_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

// 显示控制函数
void lcd_init_sequence(void);
void lcd_fill_screen(uint16_t color);
void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void lcd_detailed_test(void);
void lcd_draw_stripes(void);
void lcd_draw_test_pattern(void);
void lcd_draw_rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color);

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif