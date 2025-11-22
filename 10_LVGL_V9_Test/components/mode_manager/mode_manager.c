#include "mode_manager.h"
#include "../button_bsp/button_bsp.h"
#include "../../main/user_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

static const char *TAG = "MODE_MANAGER";

static device_mode_t current_mode = MODE_CLOCK;
static lv_obj_t *mode_label = NULL;
static lv_obj_t *mode_container = NULL;

static const char* mode_names[MODE_COUNT] = {
    "Network Config",
    "Clock Mode", 
    "Weather Mode",
    "Gallery Mode",
    "Virtual Keyboard"
};

static void create_mode_ui(void)
{
    ESP_LOGI(TAG, "Creating mode management UI");
    
    // 创建主容器
    mode_container = lv_obj_create(lv_scr_act());
    if (mode_container == NULL) {
        ESP_LOGE(TAG, "Failed to create container");
        return;
    }
    
    lv_obj_set_size(mode_container, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    lv_obj_set_style_bg_color(mode_container, lv_color_black(), 0);
    lv_obj_clear_flag(mode_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(mode_container);
    
    // 创建模式显示标签
    mode_label = lv_label_create(mode_container);
    if (mode_label == NULL) {
        ESP_LOGE(TAG, "Failed to create label");
        return;
    }
    
    lv_obj_set_style_text_color(mode_label, lv_color_white(), 0);
    // 使用默认字体，避免字体未定义错误
    lv_obj_center(mode_label);
    
    ESP_LOGI(TAG, "Mode management UI created");
}

static void update_mode_display(void)
{
    if (mode_label == NULL) {
        ESP_LOGW(TAG, "Mode label not initialized");
        return;
    }
    
    // 更新标签文本
    lv_label_set_text_fmt(mode_label, "%s\n\n%d/%d", 
                         get_mode_name(current_mode),
                         current_mode + 1, 
                         MODE_COUNT);
    
    // 设置背景颜色
    lv_color_t bg_colors[MODE_COUNT] = {
        lv_color_hex(0x2D5A78),  // 配网模式 - 蓝色
        lv_color_hex(0x4A4A4A),  // 时钟模式 - 灰色
        lv_color_hex(0x2D784A),  // 天气模式 - 绿色
        lv_color_hex(0x782D5A),  // 相册模式 - 紫色
        lv_color_hex(0x785A2D)   // 键盘模式 - 棕色
    };
    
    lv_obj_set_style_bg_color(mode_container, bg_colors[current_mode], 0);
    
    ESP_LOGI(TAG, "Updated display: %s", get_mode_name(current_mode));
}

void mode_manager_init(void)
{
    ESP_LOGI(TAG, "Starting mode manager initialization");
    
    // 创建UI
    create_mode_ui();
    
    // 更新显示
    update_mode_display();
    
    ESP_LOGI(TAG, "Mode manager initialized, current mode: %s", get_mode_name(current_mode));
}

void mode_switch_next(void)
{
    device_mode_t previous_mode = current_mode;
    current_mode = (current_mode + 1) % MODE_COUNT;
    
    ESP_LOGI(TAG, "Mode switch: %s -> %s", 
             get_mode_name(previous_mode), get_mode_name(current_mode));
    
    update_mode_display();
}

void mode_switch_to(device_mode_t mode)
{
    if (mode < MODE_COUNT) {
        device_mode_t previous_mode = current_mode;
        current_mode = mode;
        
        ESP_LOGI(TAG, "Mode jump: %s -> %s", 
                 get_mode_name(previous_mode), get_mode_name(current_mode));
        
        update_mode_display();
    } else {
        ESP_LOGW(TAG, "Invalid mode: %d", mode);
    }
}

device_mode_t get_current_mode(void)
{
    return current_mode;
}

const char* get_mode_name(device_mode_t mode)
{
    if (mode < MODE_COUNT) {
        return mode_names[mode];
    }
    return "Unknown Mode";
}

void mode_manager_task(void *arg)
{
    ESP_LOGI(TAG, "Mode manager task started");
    
    button_event_t button_event;
    QueueHandle_t button_queue = get_button_event_queue();
    
    if (button_queue == NULL) {
        ESP_LOGE(TAG, "Button queue not initialized");
        vTaskDelete(NULL);
        return;
    }
    
    while (1) {
        if (xQueueReceive(button_queue, &button_event, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "Received button event: %d", button_event);
            
            if (button_event == BUTTON_SHORT_PRESS) {
                mode_switch_next();
            } else if (button_event == BUTTON_LONG_PRESS) {
                mode_switch_to(MODE_NETWORK_CONFIG);
            }
        }
        
        // 短暂延时，让出CPU
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}