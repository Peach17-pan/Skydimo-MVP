// ui_manager.c
#include "ui_manager.h"
#include "esp_lcd_axs15231b.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "lvgl.h"
#include <string.h>

static const char *TAG = "ui_manager";

static SemaphoreHandle_t lvgl_mutex = NULL;
static lv_obj_t *current_screen = NULL;

// 颜色定义
static lv_color_t mode_colors[MODE_MAX] = {
    LV_COLOR_MAKE(0x00, 0x7A, 0xFF),  // MODE_WIFI - 蓝色
    LV_COLOR_MAKE(0x4C, 0xAF, 0x50),  // MODE_CLOCK - 绿色
    LV_COLOR_MAKE(0x00, 0xBC, 0xD4),  // MODE_WEATHER - 青色
    LV_COLOR_MAKE(0x9C, 0x27, 0xB0),  // MODE_GALLERY - 紫色
    LV_COLOR_MAKE(0xFF, 0x98, 0x00),  // MODE_KEYBOARD - 橙色
};

// 模式名称
static const char* mode_names[MODE_MAX] = {
    "WiFi配网模式",
    "时钟模式",
    "天气模式", 
    "相册模式",
    "虚拟键盘模式"
};

// LVGL刷新回调
static void lvgl_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    system_handle_t *sys_handle = (system_handle_t *)lv_display_get_user_data(disp);
    
    int offsetx1 = area->x1;
    int offsetx2 = area->x2 + 1;
    int offsety1 = area->y1;
    int offsety2 = area->y2 + 1;
    
    esp_lcd_panel_draw_bitmap(sys_handle->panel_handle, offsetx1, offsety1, offsetx2, offsety2, (void *)px_map);
    lv_display_flush_ready(disp);
}

// 触摸输入回调
static void touchpad_read_cb(lv_indev_t * indev, lv_indev_data_t * data)
{
    // 这里先模拟触摸数据，后续任务会实现真实触摸
    static int16_t last_x = 0;
    static int16_t last_y = 0;
    
    data->point.x = last_x;
    data->point.y = last_y;
    data->state = LV_INDEV_STATE_RELEASED;
    
    // 简单模拟触摸移动
    last_x = (last_x + 10) % EXAMPLE_LCD_H_RES;
    last_y = (last_y + 10) % EXAMPLE_LCD_V_RES;
}

// 创建WiFi模式界面
void ui_create_wifi_screen(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, mode_colors[MODE_WIFI], 0);
    
    // 标题
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "WiFi配网模式");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);
    
    // 状态提示
    lv_obj_t *status = lv_label_create(parent);
    lv_label_set_text(status, "长按进入配网设置");
    lv_obj_set_style_text_color(status, lv_color_white(), 0);
    lv_obj_align(status, LV_ALIGN_CENTER, 0, 0);
    
    // 图标
    lv_obj_t *icon = lv_label_create(parent);
    lv_label_set_text(icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(icon, lv_color_white(), 0);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_48, 0);
    lv_obj_align(icon, LV_ALIGN_BOTTOM_MID, 0, -40);
}

// 创建时钟模式界面
void ui_create_clock_screen(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, mode_colors[MODE_CLOCK], 0);
    
    // 标题
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "时钟模式");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);
    
    // 时间显示
    lv_obj_t *time_label = lv_label_create(parent);
    lv_label_set_text(time_label, "12:00:00");
    lv_obj_set_style_text_color(time_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_36, 0);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, 0);
    
    // 日期显示
    lv_obj_t *date_label = lv_label_create(parent);
    lv_label_set_text(date_label, "2024-01-01");
    lv_obj_set_style_text_color(date_label, lv_color_white(), 0);
    lv_obj_align(date_label, LV_ALIGN_BOTTOM_MID, 0, -40);
}

// 创建天气模式界面
void ui_create_weather_screen(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, mode_colors[MODE_WEATHER], 0);
    
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "天气模式");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);
    
    // 天气图标
    lv_obj_t *weather_icon = lv_label_create(parent);
    lv_label_set_text(weather_icon, LV_SYMBOL_SUN);
    lv_obj_set_style_text_color(weather_icon, lv_color_white(), 0);
    lv_obj_set_style_text_font(weather_icon, &lv_font_montserrat_48, 0);
    lv_obj_align(weather_icon, LV_ALIGN_CENTER, 0, -20);
    
    // 温度显示
    lv_obj_t *temp_label = lv_label_create(parent);
    lv_label_set_text(temp_label, "25°C");
    lv_obj_set_style_text_color(temp_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_36, 0);
    lv_obj_align(temp_label, LV_ALIGN_CENTER, 0, 40);
}

// 创建相册模式界面
void ui_create_gallery_screen(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, mode_colors[MODE_GALLERY], 0);
    
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "相册模式");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);
    
    // 相册图标
    lv_obj_t *gallery_icon = lv_label_create(parent);
    lv_label_set_text(gallery_icon, LV_SYMBOL_IMAGE);
    lv_obj_set_style_text_color(gallery_icon, lv_color_white(), 0);
    lv_obj_set_style_text_font(gallery_icon, &lv_font_montserrat_48, 0);
    lv_obj_align(gallery_icon, LV_ALIGN_CENTER, 0, 0);
    
    // 提示文字
    lv_obj_t *hint = lv_label_create(parent);
    lv_label_set_text(hint, "左右滑动切换图片");
    lv_obj_set_style_text_color(hint, lv_color_white(), 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -40);
}

// 创建虚拟键盘模式界面
void ui_create_keyboard_screen(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, mode_colors[MODE_KEYBOARD], 0);
    
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "虚拟键盘模式");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);
    
    // 键盘图标
    lv_obj_t *keyboard_icon = lv_label_create(parent);
    lv_label_set_text(keyboard_icon, LV_SYMBOL_KEYBOARD);
    lv_obj_set_style_text_color(keyboard_icon, lv_color_white(), 0);
    lv_obj_set_style_text_font(keyboard_icon, &lv_font_montserrat_48, 0);
    lv_obj_align(keyboard_icon, LV_ALIGN_CENTER, 0, 0);
    
    // 提示文字
    lv_obj_t *hint = lv_label_create(parent);
    lv_label_set_text(hint, "触摸按键控制PC");
    lv_obj_set_style_text_color(hint, lv_color_white(), 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -40);
}

// 切换屏幕
static void ui_switch_screen(system_handle_t *sys_handle, system_mode_t mode)
{
    if (xSemaphoreTake(lvgl_mutex, portMAX_DELAY)) {
        // 删除旧屏幕
        if (current_screen) {
            lv_obj_del(current_screen);
        }
        
        // 创建新屏幕
        current_screen = lv_obj_create(NULL);
        
        // 根据模式创建对应界面
        switch (mode) {
            case MODE_WIFI:
                ui_create_wifi_screen(current_screen);
                break;
            case MODE_CLOCK:
                ui_create_clock_screen(current_screen);
                break;
            case MODE_WEATHER:
                ui_create_weather_screen(current_screen);
                break;
            case MODE_GALLERY:
                ui_create_gallery_screen(current_screen);
                break;
            case MODE_KEYBOARD:
                ui_create_keyboard_screen(current_screen);
                break;
            default:
                break;
        }
        
        // 加载屏幕
        lv_scr_load(current_screen);
        xSemaphoreGive(lvgl_mutex);
    }
}

// LVGL定时器回调
static void lvgl_tick_cb(void *arg)
{
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

// LVGL任务
static void lvgl_task(void *arg)
{
    while (1) {
        if (xSemaphoreTake(lvgl_mutex, portMAX_DELAY)) {
            lv_timer_handler();
            xSemaphoreGive(lvgl_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

esp_err_t ui_manager_init(system_handle_t *sys_handle)
{
    ESP_LOGI(TAG, "初始化UI管理器");
    
    // 创建LVGL互斥锁
    lvgl_mutex = xSemaphoreCreateMutex();
    if (!lvgl_mutex) {
        ESP_LOGE(TAG, "创建LVGL互斥锁失败");
        return ESP_FAIL;
    }
    
    // 初始化LVGL
    lv_init();
    
    // 创建显示缓冲区
    static lv_color_t *buf1 = NULL;
    static lv_color_t *buf2 = NULL;
    size_t buffer_size = EXAMPLE_LCD_H_RES * 40 * sizeof(lv_color_t);
    
    buf1 = heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
    buf2 = heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
    
    if (!buf1 || !buf2) {
        ESP_LOGE(TAG, "分配显示缓冲区失败");
        return ESP_FAIL;
    }
    
    // 创建显示设备
    lv_display_t *disp = lv_display_create(EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    lv_display_set_flush_cb(disp, lvgl_flush_cb);
    lv_display_set_buffers(disp, buf1, buf2, buffer_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_user_data(disp, sys_handle);
    
    // 创建输入设备
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touchpad_read_cb);
    
    sys_handle->lv_display = disp;
    sys_handle->touch_indev = indev;
    
    // 创建LVGL定时器
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &lvgl_tick_cb,
        .name = "lvgl_tick"
    };
    
    esp_timer_handle_t lvgl_tick_timer = NULL;
    esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
    esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000);
    
    // 创建LVGL任务
    xTaskCreate(lvgl_task, "LVGL", LVGL_TASK_STACK_SIZE, NULL, LVGL_TASK_PRIORITY, NULL);
    
    // 创建初始屏幕
    ui_switch_screen(sys_handle, sys_handle->current_mode);
    
    ESP_LOGI(TAG, "UI管理器初始化完成");
    return ESP_OK;
}

void ui_show_current_mode(system_handle_t *sys_handle)
{
    ESP_LOGI(TAG, "切换到模式: %s", ui_get_mode_name(sys_handle->current_mode));
    ui_switch_screen(sys_handle, sys_handle->current_mode);
    sys_handle->mode_start_time = xTaskGetTickCount();
}

void ui_handle_touch(system_handle_t *sys_handle, uint16_t x, uint16_t y)
{
    ESP_LOGI(TAG, "触摸事件: (%d, %d), 模式: %s", x, y, ui_get_mode_name(sys_handle->current_mode));
}

void ui_handle_long_press(system_handle_t *sys_handle)
{
    ESP_LOGI(TAG, "长按操作 - 进入配网模式");
    sys_handle->current_mode = MODE_WIFI;
    ui_show_current_mode(sys_handle);
}

const char* ui_get_mode_name(system_mode_t mode)
{
    return (mode < MODE_MAX) ? mode_names[mode] : "未知模式";
}