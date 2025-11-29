/**
 * @file captive_portal_bsp.h
 * @brief Captive Portal Web 配网模块头文件
 * 
 * 提供 SoftAP + Web 配网功能，支持 Captive Portal 自动弹出配网页面
 */

#ifndef CAPTIVE_PORTAL_BSP_H
#define CAPTIVE_PORTAL_BSP_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief WiFi 扫描结果
 */
typedef struct {
    char ssid[33];          /* SSID 名称 */
    int8_t rssi;            /* 信号强度 */
    uint8_t authmode;       /* 认证模式 */
} wifi_scan_result_t;

/**
 * @brief 配网状态枚举
 */
typedef enum {
    PORTAL_STATE_IDLE = 0,      /* 空闲状态 */
    PORTAL_STATE_AP_STARTED,    /* AP 已启动 */
    PORTAL_STATE_WEB_READY,     /* Web 服务器就绪 */
    PORTAL_STATE_STA_CONNECTED, /* 有设备连接到 AP */
    PORTAL_STATE_CONFIGURING,   /* 正在配置 */
    PORTAL_STATE_CONNECTING,    /* 正在连接目标 WiFi */
    PORTAL_STATE_CONNECTED,     /* 已连接到目标 WiFi */
    PORTAL_STATE_FAILED,        /* 连接失败 */
} captive_portal_state_t;

/**
 * @brief 配网状态回调函数类型
 */
typedef void (*captive_portal_state_cb_t)(captive_portal_state_t state, void *user_data);

/**
 * @brief 配网配置结构体
 */
typedef struct {
    const char *ap_ssid;                /* AP 的 SSID，NULL 则使用默认值 */
    const char *ap_password;            /* AP 的密码，NULL 或少于8位则无密码 */
    uint8_t ap_channel;                 /* AP 信道，0 则使用默认值 1 */
    captive_portal_state_cb_t state_cb; /* 状态回调函数 */
    void *user_data;                    /* 用户数据，传递给回调函数 */
} captive_portal_config_t;

/**
 * @brief 初始化 Captive Portal 模块
 * @param config 配置参数
 * @return ESP_OK 成功，其他值表示失败
 */
esp_err_t captive_portal_init(const captive_portal_config_t *config);

/**
 * @brief 启动 Captive Portal（启动 SoftAP 和 Web 服务器）
 * @return ESP_OK 成功，其他值表示失败
 */
esp_err_t captive_portal_start(void);

/**
 * @brief 停止 Captive Portal
 * @return ESP_OK 成功，其他值表示失败
 */
esp_err_t captive_portal_stop(void);

/**
 * @brief 获取当前状态
 * @return 当前状态
 */
captive_portal_state_t captive_portal_get_state(void);

/**
 * @brief 检查是否处于活动状态
 * @return true 处于活动状态
 */
bool captive_portal_is_active(void);

/**
 * @brief 获取 AP 的 SSID
 * @return SSID 字符串
 */
const char* captive_portal_get_ap_ssid(void);

/**
 * @brief 获取 AP 的 IP 地址
 * @return IP 地址字符串
 */
const char* captive_portal_get_ap_ip(void);

/**
 * @brief 获取已连接设备数量
 * @return 连接的设备数
 */
uint8_t captive_portal_get_sta_count(void);

/**
 * @brief 扫描周围的 WiFi 网络
 * @param results 结果数组
 * @param max_results 最大结果数
 * @return 扫描到的网络数量
 */
uint16_t captive_portal_scan_wifi(wifi_scan_result_t *results, uint16_t max_results);

/**
 * @brief 连接到指定的 WiFi 网络
 * @param ssid 目标 SSID
 * @param password 目标密码
 * @return ESP_OK 成功开始连接
 */
esp_err_t captive_portal_connect_wifi(const char *ssid, const char *password);

/**
 * @brief 获取已保存的 WiFi SSID
 * @param ssid 输出缓冲区
 * @param max_len 缓冲区最大长度
 * @return ESP_OK 成功
 */
esp_err_t captive_portal_get_saved_ssid(char *ssid, size_t max_len);

/**
 * @brief 检查是否有已保存的 WiFi 配置
 * @return true 有保存的配置
 */
bool captive_portal_has_saved_config(void);

/**
 * @brief 清除已保存的 WiFi 配置
 * @return ESP_OK 成功
 */
esp_err_t captive_portal_clear_config(void);

/**
 * @brief 自动连接到已保存的 WiFi（如果存在）
 * @return ESP_OK 开始连接，ESP_ERR_NOT_FOUND 无保存配置
 */
esp_err_t captive_portal_auto_connect(void);

#ifdef __cplusplus
}
#endif

#endif /* CAPTIVE_PORTAL_BSP_H */

