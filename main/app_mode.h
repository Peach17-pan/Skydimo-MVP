/**
 * @file app_mode.h
 * @brief 应用模式管理模块
 */

#ifndef APP_MODE_H
#define APP_MODE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 应用模式枚举
 */
typedef enum {
    APP_MODE_WIFI_CONFIG = 0,   /* 配网模式 */
    APP_MODE_CLOCK,             /* 时钟模式 */
    APP_MODE_WEATHER,           /* 天气模式 */
    APP_MODE_PHOTO,             /* 相册模式 */
    APP_MODE_KEYBOARD,          /* 虚拟键盘模式 */
    APP_MODE_COUNT              /* 模式总数 */
} app_mode_t;

/**
 * @brief 获取当前模式
 * @return 当前应用模式
 */
app_mode_t app_mode_get_current(void);

/**
 * @brief 设置当前模式
 * @param mode 目标模式
 */
void app_mode_set(app_mode_t mode);

/**
 * @brief 切换到下一个模式（循环）
 */
void app_mode_next(void);

/**
 * @brief 切换到上一个模式（循环）
 */
void app_mode_prev(void);

/**
 * @brief 检查模式是否已变化
 * @return true 如果模式已变化
 */
bool app_mode_is_changed(void);

/**
 * @brief 清除模式变化标志
 */
void app_mode_clear_changed(void);

/**
 * @brief 获取模式名称
 * @param mode 模式
 * @return 模式名称字符串
 */
const char* app_mode_get_name(app_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif /* APP_MODE_H */

