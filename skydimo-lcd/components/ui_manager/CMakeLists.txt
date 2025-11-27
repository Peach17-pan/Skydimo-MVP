# UI 管理器组件
idf_component_register(SRCS "ui_manager.c"
                    INCLUDE_DIRS "." "../../main"
                    REQUIRES 
                    driver 
                    lcd_driver 
                    esp_lcd 
                    lvgl
                    esp_timer
                    freertos)