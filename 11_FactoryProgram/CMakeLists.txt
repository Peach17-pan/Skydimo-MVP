cmake_minimum_required(VERSION 3.16)

# 强制禁用组件管理器
set(ENV{IDF_COMPONENT_MANAGER} "0")

# 明确指定组件目录，排除有问题的组件
set(EXTRA_COMPONENT_DIRS 
    components/user_app
    components/button_bsp
    components/ui_bsp
    components/lvgl
    components/adc_bsp
    components/ble_scan_bsp
    components/esp_wifi_bsp
    components/i2c_bsp
    components/i2c_equipment
    components/lcd_bl_pwm_bsp
    components/sdcard_bsp
    components/SensorLib
    # 排除有问题的组件
    # components/audio_bsp
    # components/codec_board
    # components/esp_codec_dev
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(11_FactoryProgram)
