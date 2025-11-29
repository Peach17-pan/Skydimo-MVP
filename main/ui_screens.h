/**
 * @file ui_screens.h
 * @brief UI 界面管理模块
 */

#ifndef UI_SCREENS_H
#define UI_SCREENS_H

#include "lvgl.h"
#include "app_mode.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief UI 管理结构体
 */
typedef struct {
    lv_obj_t *main_container;       /* 主容器 */
    lv_obj_t *mode_label;           /* 模式标签 */
    lv_obj_t *content_container;    /* 内容容器 */
    
    /* 各模式的内容面板 */
    lv_obj_t *wifi_panel;           /* 配网界面 */
    lv_obj_t *clock_panel;          /* 时钟界面 */
    lv_obj_t *weather_panel;        /* 天气界面 */
    lv_obj_t *photo_panel;          /* 相册界面 */
    lv_obj_t *keyboard_panel;       /* 虚拟键盘界面 */
    
    /* 时钟相关 */
    lv_obj_t *clock_time_label;     /* 时间标签 */
    lv_obj_t *clock_date_label;     /* 日期标签 */
    
    /* 天气相关 */
    lv_obj_t *weather_temp_label;   /* 温度标签 */
    lv_obj_t *weather_desc_label;   /* 天气描述 */
    lv_obj_t *weather_icon;         /* 天气图标 */
    
    /* 配网相关 */
    lv_obj_t *wifi_status_label;    /* WiFi状态 */
    lv_obj_t *wifi_ssid_label;      /* SSID标签 */
    lv_obj_t *wifi_ap_ssid_label;   /* AP SSID 标签 */
    lv_obj_t *wifi_ap_ip_label;     /* AP IP 标签 */
    lv_obj_t *wifi_hint_label;      /* 提示信息标签 */
    
    /* 相册相关 */
    lv_obj_t *photo_image;          /* 图片显示 */
    lv_obj_t *photo_info_label;     /* 图片信息 */
    
    /* 键盘相关 */
    lv_obj_t *keyboard_ta;          /* 文本输入区 */
    lv_obj_t *keyboard_obj;         /* 键盘对象 */
} ui_manager_t;

/**
 * @brief 初始化 UI 界面
 * @return UI 管理器指针
 */
ui_manager_t* ui_screens_init(void);

/**
 * @brief 切换到指定模式界面
 * @param mode 目标模式
 */
void ui_screens_switch_to(app_mode_t mode);

/**
 * @brief 更新时钟显示
 * @param hour 小时
 * @param minute 分钟
 * @param second 秒
 */
void ui_clock_update_time(uint8_t hour, uint8_t minute, uint8_t second);

/**
 * @brief 更新日期显示
 * @param year 年
 * @param month 月
 * @param day 日
 */
void ui_clock_update_date(uint16_t year, uint8_t month, uint8_t day);

/**
 * @brief 更新天气显示
 * @param temp 温度
 * @param desc 天气描述
 */
void ui_weather_update(int temp, const char *desc);

/**
 * @brief 更新 WiFi 状态
 * @param connected 是否已连接（影响显示颜色）
 * @param message 状态消息文本
 */
void ui_wifi_update_status(bool connected, const char *message);

/**
 * @brief 设置配网模式
 * @param provision_mode true 进入配网模式
 */
void ui_wifi_set_provision_mode(bool provision_mode);

/**
 * @brief 更新配网信息显示
 * @param ap_ssid AP 的 SSID
 * @param ap_ip AP 的 IP 地址
 */
void ui_wifi_update_provision_info(const char *ap_ssid, const char *ap_ip);

/**
 * @brief 获取 UI 管理器
 * @return UI 管理器指针
 */
ui_manager_t* ui_get_manager(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_SCREENS_H */

