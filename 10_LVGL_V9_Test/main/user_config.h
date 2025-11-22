#ifndef USER_CONFIG_H
#define USER_CONFIG_H

#include <stdint.h>

// I2C 配置
#define ESP32_SCL_NUM (GPIO_NUM_48)
#define ESP32_SDA_NUM (GPIO_NUM_47)

// Touch I2C - 微雪3.49寸屏触摸配置
#define Touch_SCL_NUM (GPIO_NUM_18)
#define Touch_SDA_NUM (GPIO_NUM_17)

// 显示配置 - 微雪3.49寸屏分辨率
#define EXAMPLE_LCD_H_RES             480
#define EXAMPLE_LCD_V_RES             320
#define LVGL_DMA_BUFF_LEN    (EXAMPLE_LCD_H_RES * 64 * 2)
#define LVGL_SPIRAM_BUFF_LEN (EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * 2)

#define USER_DISP_ROT_90    1
#define USER_DISP_ROT_NONO  0
#define Rotated USER_DISP_ROT_90  // 微雪屏通常需要旋转90度

// LCD 引脚定义 - 微雪3.49寸屏引脚（根据实际硬件调整）
#define EXAMPLE_PIN_NUM_LCD_CS            (GPIO_NUM_10)
#define EXAMPLE_PIN_NUM_LCD_PCLK          (GPIO_NUM_12) 
#define EXAMPLE_PIN_NUM_LCD_DATA0         (GPIO_NUM_11)  // MOSI
#define EXAMPLE_PIN_NUM_LCD_DC            (GPIO_NUM_8)   // DC 引脚
#define EXAMPLE_PIN_NUM_LCD_RST           (GPIO_NUM_9)   // 复位引脚
#define EXAMPLE_PIN_NUM_BK_LIGHT          (GPIO_NUM_46)  // 背光控制

// 触摸配置
#define DISP_TOUCH_ADDR                   0x14  // GT911默认地址
#define EXAMPLE_PIN_NUM_TOUCH_RST         (GPIO_NUM_16)
#define EXAMPLE_PIN_NUM_TOUCH_INT         (GPIO_NUM_21)

// 按键配置
#define EXAMPLE_PIN_NUM_KEY (GPIO_NUM_0)

// SPI 配置
#define LCD_HOST SPI2_HOST

#endif