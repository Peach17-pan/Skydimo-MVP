#ifndef USER_CONFIG_H
#define USER_CONFIG_H

// SPI handle
#define SDSPI_HOST SPI2_HOST
#define LCD_HOST SPI3_HOST

// ESP32 I2C
#define ESP32_SCL_NUM (GPIO_NUM_48)
#define ESP32_SDA_NUM (GPIO_NUM_47)

// Touch I2C
#define Touch_SCL_NUM (GPIO_NUM_18)
#define Touch_SDA_NUM (GPIO_NUM_17)

// DISP - 调整分辨率设置和缓冲区大小
#define EXAMPLE_LCD_H_RES              172
#define EXAMPLE_LCD_V_RES              480  // 修正为480而不是640

// 减小缓冲区大小以节省内存
#define LVGL_DMA_BUFF_LEN    (EXAMPLE_LCD_H_RES * 20 * 2)   // 减小DMA缓冲区
#define LVGL_SPIRAM_BUFF_LEN (EXAMPLE_LCD_H_RES * 40 * 2)   // 减小SPIRAM缓冲区

#define USER_DISP_ROT_90    1
#define USER_DISP_ROT_NONO  0
#define Rotated USER_DISP_ROT_NONO

/*bl test*/
#define Backlight_Testing 0

#define EXAMPLE_PIN_NUM_LCD_CS            (GPIO_NUM_9)
#define EXAMPLE_PIN_NUM_LCD_PCLK          (GPIO_NUM_10) 
#define EXAMPLE_PIN_NUM_LCD_DATA0         (GPIO_NUM_11)
#define EXAMPLE_PIN_NUM_LCD_DATA1         (GPIO_NUM_12)
#define EXAMPLE_PIN_NUM_LCD_DATA2         (GPIO_NUM_13)
#define EXAMPLE_PIN_NUM_LCD_DATA3         (GPIO_NUM_14)
#define EXAMPLE_PIN_NUM_LCD_RST           (GPIO_NUM_21)
#define EXAMPLE_PIN_NUM_BK_LIGHT          (GPIO_NUM_8)

#define DISP_TOUCH_ADDR                   0x3B
#define EXAMPLE_PIN_NUM_TOUCH_RST         (-1)
#define EXAMPLE_PIN_NUM_TOUCH_INT         (-1)

// 按键配置
#define EXAMPLE_PIN_NUM_KEY               (GPIO_NUM_0)
#define KEY_SHORT_PRESS_TIME_MS           50
#define KEY_LONG_PRESS_TIME_MS            2000

// 设备模式定义
typedef enum {
    MODE_NETWORK_CONFIG = 0,
    MODE_CLOCK,              
    MODE_WEATHER,            
    MODE_GALLERY,            
    MODE_VIRTUAL_KEYBOARD,   
    MODE_MAX
} device_mode_t;

#endif