cmake_minimum_required(VERSION 3.5)

idf_component_register(
    SRCS "button_bsp.c"
    INCLUDE_DIRS "."
    REQUIRES 
        driver
        esp_timer
    PRIV_REQUIRES main
)