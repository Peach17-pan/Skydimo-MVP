#ifndef USER_CONFIG_H
#define USER_CONFIG_H

//spi & i2c handle
#define LCD_HOST SPI3_HOST

// touch I2C port
#define Touch_SCL_NUM (GPIO_NUM_18)
#define Touch_SDA_NUM (GPIO_NUM_17)

// touch esp
#define ESP_SCL_NUM (GPIO_NUM_48)
#define ESP_SDA_NUM (GPIO_NUM_47)

//  DISP
#define EXAMPLE_PIN_NUM_LCD_CS     (GPIO_NUM_9) 
#define EXAMPLE_PIN_NUM_LCD_PCLK   (GPIO_NUM_10)
#define EXAMPLE_PIN_NUM_LCD_DATA0  (GPIO_NUM_11)
#define EXAMPLE_PIN_NUM_LCD_DATA1  (GPIO_NUM_12)
#define EXAMPLE_PIN_NUM_LCD_DATA2  (GPIO_NUM_13)
#define EXAMPLE_PIN_NUM_LCD_DATA3  (GPIO_NUM_14)
#define EXAMPLE_PIN_NUM_LCD_RST    (GPIO_NUM_21)
#define EXAMPLE_PIN_NUM_BK_LIGHT   (GPIO_NUM_8) 

// 按键配置
#define EXAMPLE_PIN_NUM_KEY               (GPIO_NUM_0)  // 按键引脚
#define KEY_SHORT_PRESS_TIME_MS           50            // 短按消抖时间
#define KEY_LONG_PRESS_TIME_MS            2000          // 长按时间

#define EXAMPLE_PIN_NUM_TOUCH_ADDR        0x3b
#define EXAMPLE_PIN_NUM_TOUCH_RST         (-1)
#define EXAMPLE_PIN_NUM_TOUCH_INT         (-1)

// 添加缺失的I2C设备地址定义
#define DISP_TOUCH_ADDR                   0x3B  // 触摸屏I2C地址

// 添加缺失的LVGL配置
#define EXAMPLE_LVGL_TICK_PERIOD_MS    2
#define EXAMPLE_LVGL_TASK_MAX_DELAY_MS 500
#define EXAMPLE_LVGL_TASK_MIN_DELAY_MS 1

/*ADDR*/
#define EXAMPLE_RTC_ADDR 0x51
#define EXAMPLE_IMU_ADDR 0x6b

#define USER_DISP_ROT_90    1
#define USER_DISP_ROT_NONO  0
#define Rotated USER_DISP_ROT_90   //软件实现旋转

/*bl test*/
#define Backlight_Testing 0

#if (Rotated == USER_DISP_ROT_NONO)
#define EXAMPLE_LCD_H_RES 172   
#define EXAMPLE_LCD_V_RES 480
#else
#define EXAMPLE_LCD_H_RES 480   
#define EXAMPLE_LCD_V_RES 172
#endif

#define LCD_NOROT_HRES     172
#define LCD_NOROT_VRES     480
#define LVGL_DMA_BUFF_LEN (LCD_NOROT_HRES * 80 * 2)
#define LVGL_SPIRAM_BUFF_LEN (EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * 2)

// 设备模式定义
typedef enum {
    MODE_NETWORK_CONFIG = 0,   // 配网模式
    MODE_CLOCK,                // 时钟模式  
    MODE_WEATHER,              // 天气模式
    MODE_GALLERY,              // 相册模式
    MODE_VIRTUAL_KEYBOARD,     // 虚拟键盘模式
    MODE_MAX
} device_mode_t;

#endif