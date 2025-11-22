#include "ui_port.h"
#include "lcd_touch.h"
#include "mode_manager.h"
#include "esp_log.h"

static const char *mode_name[] = {"时钟","天气","相册","键盘","配网"};
static const uint16_t mode_bg_color[] = {
    0xF800, 0x07E0, 0x001F, 0xFFE0, 0xF81F
};

void ui_switch(mode_t m)
{
    lcd_fill_color(mode_bg_color[m]);  
    lcd_print_mode(mode_name[m]);     
}