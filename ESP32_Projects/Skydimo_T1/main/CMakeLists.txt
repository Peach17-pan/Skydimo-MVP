# 主组件配置
idf_component_register(
    SRCS "main.c" 
         "i2c_bsp.c" 
         "lcd_bl_pwm_bsp.c" 
         "mode_manager.c" 
         "key_detector.c" 
         "ui_manager.c"
    INCLUDE_DIRS "."
)

# LVGL配置
target_compile_definitions(${COMPONENT_LIB} PRIVATE 
    LV_LVGL_H_INCLUDE_SIMPLE
    LV_CONF_INCLUDE_SIMPLE
)

# 包含必要的LVGL源文件
set(LVGL_SOURCES
    ${LVGL_DIR}/src/core/lv_obj.c
    ${LVGL_DIR}/src/core/lv_group.c
    ${LVGL_DIR}/src/core/lv_refr.c
    ${LVGL_DIR}/src/core/lv_obj_class.c
    ${LVGL_DIR}/src/core/lv_obj_draw.c
    ${LVGL_DIR}/src/core/lv_obj_event.c
    ${LVGL_DIR}/src/core/lv_obj_pos.c
    ${LVGL_DIR}/src/core/lv_obj_style.c
    ${LVGL_DIR}/src/core/lv_obj_tree.c
    ${LVGL_DIR}/src/misc/lv_color.c
    ${LVGL_DIR}/src/misc/lv_area.c
    ${LVGL_DIR}/src/misc/lv_anim.c
    ${LVGL_DIR}/src/misc/lv_timer.c
    ${LVGL_DIR}/src/hal/lv_hal_disp.c
    ${LVGL_DIR}/src/hal/lv_hal_indev.c
    ${LVGL_DIR}/src/hal/lv_hal_tick.c
    ${LVGL_DIR}/src/widgets/lv_label.c
    ${LVGL_DIR}/src/widgets/lv_obj.c
    ${LVGL_DIR}/src/font/lv_font.c
    ${LVGL_DIR}/src/font/lv_font_montserrat_12.c
    ${LVGL_DIR}/src/font/lv_font_montserrat_14.c
    ${LVGL_DIR}/src/font/lv_font_montserrat_16.c
    ${LVGL_DIR}/src/draw/lv_draw.c
    ${LVGL_DIR}/src/draw/lv_draw_rect.c
    ${LVGL_DIR}/src/draw/lv_draw_label.c
    ${LVGL_DIR}/src/display/lv_display.c
    ${LVGL_DIR}/src/indev/lv_indev.c
)

# 添加LVGL源文件
foreach(source_file ${LVGL_SOURCES})
    if(EXISTS ${source_file})
        target_sources(${COMPONENT_LIB} PRIVATE ${source_file})
    else()
        # 尝试在components目录下查找
        string(REPLACE "${LVGL_DIR}" "../components/lvgl" alt_source_file ${source_file})
        if(EXISTS ${alt_source_file})
            target_sources(${COMPONENT_LIB} PRIVATE ${alt_source_file})
        else()
            message(WARNING "LVGL source file not found: ${source_file}")
        endif()
    endif()
endforeach()