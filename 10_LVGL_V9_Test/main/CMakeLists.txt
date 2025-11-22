cmake_minimum_required(VERSION 3.5)

idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES 
        i2c_bsp 
        lcd_bl_pwm_bsp 
        button_bsp 
        mode_manager
        lcd_driver  # 添加 LCD 驱动依赖
)