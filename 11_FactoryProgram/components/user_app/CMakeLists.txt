# 正确的 user_app/CMakeLists.txt
file(GLOB_RECURSE srcs ./*.c ./*.cpp)

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "."
    REQUIRES lvgl button_bsp adc_bsp i2c_equipment lcd_bl_pwm_bsp esp_wifi_bsp i2c_bsp sdcard_bsp ble_scan_bsp
)