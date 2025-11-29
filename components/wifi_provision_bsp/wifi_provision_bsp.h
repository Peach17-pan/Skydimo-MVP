/**
 * @file wifi_provision_bsp.h
 * @brief WiFi 配网模块头文件
 * 
 * 提供 SoftAP 配网功能，支持通过 Web 页面配置 WiFi
 */

#ifndef WIFI_PROVISION_BSP_H
#define WIFI_PROVISION_BSP_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 配网状态枚举
 */
typedef enum {
    WIFI_PROV_STATE_IDLE = 0,       /* 空闲状态 */
    WIFI_PROV_STATE_AP_STARTED,     /* AP 已启动 */
    WIFI_PROV_STATE_STA_CONNECTED,  /* 有设备连接到 AP */
    WIFI_PROV_STATE_CONNECTING,     /* 正在连接目标 WiFi */
    WIFI_PROV_STATE_CONNECTED,      /* 已连接到目标 WiFi */
    WIFI_PROV_STATE_FAILED,         /* 连接失败 */
} wifi_prov_state_t;

/**
 * @brief 配网状态回调函数类型
 */
typedef void (*wifi_prov_state_cb_t)(wifi_prov_state_t state, void *user_data);

/**
 * @brief 配网配置结构体
 */
typedef struct {
    const char *ap_ssid;            /* AP 的 SSID，NULL 则使用默认值 */
    const char *ap_password;        /* AP 的密码，NULL 则无密码 */
    wifi_prov_state_cb_t state_cb;  /* 状态回调函数 */
    void *user_data;                /* 用户数据，传递给回调函数 */
} wifi_prov_config_t;

/**
 * @brief 初始化 WiFi 配网模块
 * @param config 配网配置
 * @return ESP_OK 成功，其他值表示失败
 */
esp_err_t wifi_provision_init(const wifi_prov_config_t *config);

/**
 * @brief 启动配网模式（启动 SoftAP）
 * @return ESP_OK 成功，其他值表示失败
 */
esp_err_t wifi_provision_start(void);

/**
 * @brief 停止配网模式
 * @return ESP_OK 成功，其他值表示失败
 */
esp_err_t wifi_provision_stop(void);

/**
 * @brief 获取当前配网状态
 * @return 当前状态
 */
wifi_prov_state_t wifi_provision_get_state(void);

/**
 * @brief 检查是否处于配网模式
 * @return true 处于配网模式
 */
bool wifi_provision_is_active(void);

/**
 * @brief 获取 AP 的 SSID
 * @return SSID 字符串
 */
const char* wifi_provision_get_ap_ssid(void);

/**
 * @brief 获取 AP 的 IP 地址
 * @return IP 地址字符串
 */
const char* wifi_provision_get_ap_ip(void);

/**
 * @brief 获取已连接设备数量
 * @return 连接的设备数
 */
uint8_t wifi_provision_get_sta_count(void);

/**
 * @brief 尝试连接到指定的 WiFi 网络
 * @param ssid 目标 SSID
 * @param password 目标密码
 * @return ESP_OK 成功开始连接，其他值表示失败
 */
esp_err_t wifi_provision_connect_sta(const char *ssid, const char *password);

/**
 * @brief 获取已保存的 WiFi SSID
 * @param ssid 输出缓冲区
 * @param max_len 缓冲区最大长度
 * @return ESP_OK 成功
 */
esp_err_t wifi_provision_get_saved_ssid(char *ssid, size_t max_len);

/**
 * @brief 检查是否有已保存的 WiFi 配置
 * @return true 有保存的配置
 */
bool wifi_provision_has_saved_config(void);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_PROVISION_BSP_H */



