/**
 * @file button_bsp.h
 * @brief 按键 BSP 头文件
 */

#ifndef BUTTON_BSP_H
#define BUTTON_BSP_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 按键 GPIO 定义 */
#define USER_KEY_BOOT   0       /* BOOT 按键 GPIO0 */
#define USER_KEY_PWR    16      /* PWR 按键 GPIO16 */

/* 事件组句柄 */
extern EventGroupHandle_t boot_key_event;
extern EventGroupHandle_t pwr_key_event;

/* 事件位定义 */
#define KEY_EVENT_SINGLE_CLICK    (1 << 0)
#define KEY_EVENT_DOUBLE_CLICK    (1 << 1)
#define KEY_EVENT_LONG_PRESS      (1 << 2)
#define KEY_EVENT_PRESS_UP        (1 << 3)

/**
 * @brief 初始化按键
 */
void button_bsp_init(void);

/**
 * @brief 获取 PWR 按键重复按下次数
 * @return 重复按下次数
 */
uint8_t button_pwr_get_repeat_count(void);

/**
 * @brief 获取 BOOT 按键重复按下次数
 * @return 重复按下次数
 */
uint8_t button_boot_get_repeat_count(void);

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_BSP_H */

