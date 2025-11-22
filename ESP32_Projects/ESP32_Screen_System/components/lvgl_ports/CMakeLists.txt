idf_component_register(
    SRCS "lv_port_disp.c"  # "lv_port_indev.c" 任务1可暂不启用
    INCLUDE_DIRS "."
    REQUIRES 
        driver 
        main 
        lvgl 
        esp_lcd
)