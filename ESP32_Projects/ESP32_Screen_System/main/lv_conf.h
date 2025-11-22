#ifndef LV_CONF_H
#define LV_CONF_H

/* Set this to 1 to use LVGL v9 features */
#define LV_USE_API_EXTENSION_V9 1

/* Color depth: 16 (RGB565) */
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0

/* Screen resolution */
#define LV_HOR_RES_MAX 320
#define LV_VER_RES_MAX 480

/* Memory settings */
#define LV_MEM_SIZE (64 * 1024)  /* 64KB */
#define LV_MEM_ADR 0

/* Tick settings */
#define LV_TICK_CUSTOM 1
#if LV_TICK_CUSTOM
    #define LV_TICK_CUSTOM_INCLUDE "esp_timer.h"
    #define LV_TICK_CUSTOM_SYS_TIME_EXPR (esp_timer_get_time() / 1000LL)
#endif

/* Log settings */
#define LV_USE_LOG 1
#if LV_USE_LOG
    #define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#endif

/* Font settings */
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/* Other important settings for v9 */
#define LV_USE_DEV_VERSION 0
#define LV_BUILD_EXAMPLES 0
#define LV_USE_DEMO_WIDGETS 0
#define LV_USE_DEMO_BENCHMARK 0
#define LV_USE_DEMO_STRESS 0

#endif /* LV_CONF_H */