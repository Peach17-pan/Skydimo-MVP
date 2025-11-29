idf_component_register(
    SRCS "main.c" "app_mode.c" "ui_screens.c"
    INCLUDE_DIRS "."
    REQUIRES captive_portal_bsp button_bsp i2c_bsp lcd_bl_pwm_bsp esp_wifi esp_netif)
