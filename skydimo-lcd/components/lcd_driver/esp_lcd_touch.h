#ifndef ESP_LCD_TOUCH_H
#define ESP_LCD_TOUCH_H

#include "esp_err.h"
#include "esp_lcd_panel_io.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void *esp_lcd_touch_handle_t;

typedef struct {
    int rst_gpio_num;
    int int_gpio_num;
    struct {
        bool reset;
        bool interrupt;
    } levels;
    void *interrupt_callback;
} esp_lcd_touch_config_t;

#ifdef __cplusplus
}
#endif

#endif /* ESP_LCD_TOUCH_H */