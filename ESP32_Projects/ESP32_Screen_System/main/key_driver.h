#ifndef KEY_DRIVER_H
#define KEY_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 按键GPIO引脚定义
#define KEY_GPIO_PIN        GPIO_NUM_0
#define LONG_PRESS_MS       2000
#define DEBOUNCE_MS         50

// 系统模式枚举
typedef enum {
    MODE_NET_CFG = 0,
    MODE_CLOCK,
    MODE_WEATHER,
    MODE_GALLERY,
    MODE_VKEYBOARD,
    MODE_MAX
} sys_mode_t;

// 模式切换回调函数类型
typedef void (*mode_change_cb_t)(sys_mode_t new_mode);

/**
 * @brief 初始化按键驱动
 * @param callback 模式切换回调函数
 */
void key_driver_init(mode_change_cb_t callback);

/**
 * @brief 按键处理任务（内部使用）
 */
void key_driver_task(void *arg);

/**
 * @brief 获取当前模式
 * @return 当前系统模式
 */
sys_mode_t key_get_current_mode(void);

#ifdef __cplusplus
}
#endif

#endif /* KEY_DRIVER_H */