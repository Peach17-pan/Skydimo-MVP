/**
 * @file app_mode.c
 * @brief 应用模式管理模块实现
 */

#include "app_mode.h"
#include "esp_log.h"

static const char *TAG = "app_mode";

/* 模式名称数组 */
static const char *mode_names[] = {
    "配网",      /* APP_MODE_WIFI_CONFIG */
    "时钟",      /* APP_MODE_CLOCK */
    "天气",      /* APP_MODE_WEATHER */
    "相册",      /* APP_MODE_PHOTO */
    "键盘",      /* APP_MODE_KEYBOARD */
};

/* 当前模式 */
static volatile app_mode_t current_mode = APP_MODE_CLOCK;
static volatile bool mode_changed = false;

app_mode_t app_mode_get_current(void)
{
    return current_mode;
}

void app_mode_set(app_mode_t mode)
{
    if (mode < APP_MODE_COUNT && mode != current_mode) {
        current_mode = mode;
        mode_changed = true;
        ESP_LOGI(TAG, "Mode changed to: %s", mode_names[mode]);
    }
}

void app_mode_next(void)
{
    app_mode_t next = (current_mode + 1) % APP_MODE_COUNT;
    app_mode_set(next);
}

void app_mode_prev(void)
{
    app_mode_t prev = (current_mode == 0) ? (APP_MODE_COUNT - 1) : (current_mode - 1);
    app_mode_set(prev);
}

bool app_mode_is_changed(void)
{
    return mode_changed;
}

void app_mode_clear_changed(void)
{
    mode_changed = false;
}

const char* app_mode_get_name(app_mode_t mode)
{
    if (mode < APP_MODE_COUNT) {
        return mode_names[mode];
    }
    return "Unknown";
}

