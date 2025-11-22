idf_component_register(
    SRCS 
        "main.c"
        "ui_mode.c"
        "key_driver.c"
    INCLUDE_DIRS "."
    REQUIRES 
        esp_driver_gpio
        lvgl
        lvgl_ports
)

# 设置LVGL配置路径
target_add_binary_data(${COMPONENT_LIB} "lv_conf.h" TEXT)