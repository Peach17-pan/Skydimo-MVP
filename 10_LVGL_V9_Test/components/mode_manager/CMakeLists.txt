cmake_minimum_required(VERSION 3.5)

idf_component_register(
    SRCS "mode_manager.c"
    INCLUDE_DIRS "."
    REQUIRES 
        button_bsp
        lvgl
    PRIV_REQUIRES main
)