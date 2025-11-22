idf_component_register(
    SRCS "lv_port_disp.c" "lv_port_indev.c" "lv_port.c"
    INCLUDE_DIRS "."
    REQUIRES lvgl lcd_driver i2c_bsp
    PRIV_REQUIRES main
)