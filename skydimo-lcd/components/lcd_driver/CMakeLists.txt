# components/lcd_driver/CMakeLists.txt 应该是这样：
idf_component_register(SRCS "esp_lcd_axs15231b.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_lcd)