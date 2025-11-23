idf_component_register(
    SRCS "main.c" 
         "i2c_bsp.c" 
         "lcd_bl_pwm_bsp.c" 
         "mode_manager.c" 
         "key_detector.c" 
         "ui_manager.c"
    INCLUDE_DIRS "."
)

# 包含 LVGL
target_include_directories(${COMPONENT_LIB} PRIVATE 
    ../components/lvgl
)

# LVGL 配置
target_compile_definitions(${COMPONENT_LIB} PRIVATE 
    LV_LVGL_H_INCLUDE_SIMPLE
    LV_CONF_INCLUDE_SIMPLE
    LV_CONF_PATH="lv_conf.h"
)

# 只添加实际存在的核心文件
set(LVGL_CORE_SOURCES
    ../components/lvgl/src/core/lv_obj.c
    ../components/lvgl/src/core/lv_group.c
    ../components/lvgl/src/core/lv_refr.c
    ../components/lvgl/src/core/lv_obj_class.c
    ../components/lvgl/src/core/lv_obj_draw.c
    ../components/lvgl/src/core/lv_obj_event.c
    ../components/lvgl/src/core/lv_obj_pos.c
    ../components/lvgl/src/core/lv_obj_scroll.c
    ../components/lvgl/src/core/lv_obj_style.c
    ../components/lvgl/src/core/lv_obj_style_gen.c
    ../components/lvgl/src/core/lv_obj_tree.c
)

# 检查并添加 misc 目录的文件
if(EXISTS "../components/lvgl/src/misc")
    file(GLOB LVGL_MISC_SOURCES "../components/lvgl/src/misc/*.c")
    list(APPEND LVGL_CORE_SOURCES ${LVGL_MISC_SOURCES})
endif()

# 检查并添加 hal 目录的文件  
if(EXISTS "../components/lvgl/src/hal")
    file(GLOB LVGL_HAL_SOURCES "../components/lvgl/src/hal/*.c")
    list(APPEND LVGL_CORE_SOURCES ${LVGL_HAL_SOURCES})
endif()

# 检查并添加 widgets 目录的文件
if(EXISTS "../components/lvgl/src/widgets")
    file(GLOB LVGL_WIDGET_SOURCES "../components/lvgl/src/widgets/*.c")
    list(APPEND LVGL_CORE_SOURCES ${LVGL_WIDGET_SOURCES})
endif()

# 检查并添加 draw 目录的文件
if(EXISTS "../components/lvgl/src/draw")
    file(GLOB LVGL_DRAW_SOURCES "../components/lvgl/src/draw/*.c")
    list(APPEND LVGL_CORE_SOURCES ${LVGL_DRAW_SOURCES})
endif()

# 检查并添加 font 目录的文件
if(EXISTS "../components/lvgl/src/font")
    file(GLOB LVGL_FONT_SOURCES "../components/lvgl/src/font/*.c")
    list(APPEND LVGL_CORE_SOURCES ${LVGL_FONT_SOURCES})
endif()

# 排除有问题的文件
list(FILTER LVGL_CORE_SOURCES EXCLUDE REGEX ".*sysmon.*")
list(FILTER LVGL_CORE_SOURCES EXCLUDE REGEX ".*observer.*")
list(FILTER LVGL_CORE_SOURCES EXCLUDE REGEX ".*monitor.*")

# 只添加实际存在的文件
foreach(source_file ${LVGL_CORE_SOURCES})
    if(EXISTS ${source_file})
        target_sources(${COMPONENT_LIB} PRIVATE ${source_file})
        message(STATUS "Adding LVGL source: ${source_file}")
    else()
        message(WARNING "LVGL source file not found: ${source_file}")
    endif()
endforeach()