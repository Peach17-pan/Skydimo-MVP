#ifndef KEY_DETECTOR_H
#define KEY_DETECTOR_H

#include "user_config.h"
#include <stdbool.h>  

#ifdef __cplusplus
extern "C" {
#endif

// 按键事件回调函数类型
typedef void (*key_event_callback_t)(void);

// 按键检测器初始化
void key_detector_init(key_event_callback_t short_press_cb, 
                      key_event_callback_t long_press_cb);

// 获取按键状态
bool is_key_pressed(void);

#ifdef __cplusplus
}
#endif

#endif