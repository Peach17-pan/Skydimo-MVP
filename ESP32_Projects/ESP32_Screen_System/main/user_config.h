#ifndef USER_CONFIG_H
#define USER_CONFIG_H

// SPI 主机配置
#define SDSPI_HOST SPI2_HOST
#define LCD_HOST SPI3_HOST

// I2C 配置
#define ESP32_SCL_NUM (GPIO_NUM_48)  // 外设 I2C 时钟
#define ESP32_SDA_NUM (GPIO_NUM_47)  // 外设 I2C 数据

// 触摸 I2C 引脚
#define Touch_SCL_NUM (GPIO_NUM_18)  // 触摸 I2C 时钟
#define Touch_SDA_NUM (GPIO_NUM_17)  // 触摸 I2C 数据

// 屏幕分辨率配置 - 微雪 3.49寸屏
#define EXAMPLE_LCD_H_RES              320
#define EXAMPLE_LCD_V_RES              480

// LVGL 缓冲区配置
#define LVGL_DMA_BUFF_LEN    (EXAMPLE_LCD_H_RES * 64 * 2)
#define LVGL_SPIRAM_BUFF_LEN (EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * 2)

// 旋转配置
#define USER_DISP_ROT_90    1
#define USER_DISP_ROT_NONO  0
#define Rotated USER_DISP_ROT_NONO   // 软件实现旋转

// 背光测试
#define Backlight_Testing 0

// LCD 引脚配置
#define EXAMPLE_PIN_NUM_LCD_CS            (GPIO_NUM_9)   // LCD 片选
#define EXAMPLE_PIN_NUM_LCD_PCLK          (GPIO_NUM_10)  // SPI 时钟
#define EXAMPLE_PIN_NUM_LCD_DATA0         (GPIO_NUM_11)  // QSPI 数据0
#define EXAMPLE_PIN_NUM_LCD_DATA1         (GPIO_NUM_12)  // QSPI 数据1
#define EXAMPLE_PIN_NUM_LCD_DATA2         (GPIO_NUM_13)  // QSPI 数据2
#define EXAMPLE_PIN_NUM_LCD_DATA3         (GPIO_NUM_14)  // QSPI 数据3
#define EXAMPLE_PIN_NUM_LCD_RST           (GPIO_NUM_21)  // LCD 复位
#define EXAMPLE_PIN_NUM_BK_LIGHT          (GPIO_NUM_8)   // 背光控制

// 触摸配置
#define DISP_TOUCH_ADDR                   0x38  // 常见触摸芯片地址
#define EXAMPLE_PIN_NUM_TOUCH_RST         (-1)
#define EXAMPLE_PIN_NUM_TOUCH_INT         (-1)

// 按键配置
#define KEY_GPIO_PIN                      (GPIO_NUM_0)   // 按键引脚

#endif