# 主组件
idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES 
                    nvs_flash 
                    driver 
                    input_handler 
                    ui_manager 
                    lcd_driver
                    lvgl
                    esp_timer
                    freertos)