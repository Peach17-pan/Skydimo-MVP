/**
 * @file device_report_bsp.h
 * @brief 设备信息上报模块头文件
 * 
 * 提供 WiFi 连接后自动向 PC 上报设备信息的功能
 * 使用 UDP 广播实现设备发现
 */

#ifndef DEVICE_REPORT_BSP_H
#define DEVICE_REPORT_BSP_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 默认配置 */
#define DEVICE_REPORT_DEFAULT_PORT      9527    /* 默认广播端口 */
#define DEVICE_REPORT_DEFAULT_INTERVAL  5000    /* 默认上报间隔 (ms) */
#define DEVICE_REPORT_MAX_RETRY         3       /* 最大重试次数 */

/**
 * @brief 上报状态枚举
 */
typedef enum {
    REPORT_STATE_IDLE = 0,      /* 空闲状态 */
    REPORT_STATE_RUNNING,       /* 正在上报 */
    REPORT_STATE_ERROR,         /* 错误状态 */
} device_report_state_t;

/**
 * @brief 上报状态回调函数类型
 */
typedef void (*device_report_state_cb_t)(device_report_state_t state, void *user_data);

/**
 * @brief 上报配置结构体
 */
typedef struct {
    const char *device_name;            /* 设备名称，NULL 使用默认值 "Skydimo" */
    const char *device_type;            /* 设备类型，NULL 使用默认值 "SK-LCD" */
    uint16_t broadcast_port;            /* 广播端口，0 使用默认值 9527 */
    uint32_t report_interval_ms;        /* 上报间隔 (ms)，0 使用默认值 5000 */
    bool broadcast_enabled;             /* 是否启用广播上报 */
    device_report_state_cb_t state_cb;  /* 状态回调函数 */
    void *user_data;                    /* 用户数据 */
} device_report_config_t;

/**
 * @brief 设备信息结构体
 */
typedef struct {
    char device_name[32];       /* 设备名称 */
    char device_type[16];       /* 设备类型 */
    char ip_address[16];        /* IP 地址 */
    char mac_address[18];       /* MAC 地址 */
    uint8_t wifi_rssi;          /* WiFi 信号强度 */
    uint32_t uptime_ms;         /* 运行时间 (ms) */
    uint16_t free_heap_kb;      /* 剩余堆内存 (KB) */
} device_info_t;

/**
 * @brief 初始化设备上报模块
 * @param config 配置参数，NULL 使用默认配置
 * @return ESP_OK 成功
 */
esp_err_t device_report_init(const device_report_config_t *config);

/**
 * @brief 启动设备上报
 * @return ESP_OK 成功
 */
esp_err_t device_report_start(void);

/**
 * @brief 停止设备上报
 * @return ESP_OK 成功
 */
esp_err_t device_report_stop(void);

/**
 * @brief 立即发送一次上报（不等待定时器）
 * @return ESP_OK 成功
 */
esp_err_t device_report_send_now(void);

/**
 * @brief 获取当前状态
 * @return 当前状态
 */
device_report_state_t device_report_get_state(void);

/**
 * @brief 获取设备信息
 * @param info 输出设备信息
 * @return ESP_OK 成功
 */
esp_err_t device_report_get_info(device_info_t *info);

/**
 * @brief 设置上报间隔
 * @param interval_ms 上报间隔 (ms)
 * @return ESP_OK 成功
 */
esp_err_t device_report_set_interval(uint32_t interval_ms);

/**
 * @brief 设置目标 PC 地址（单播模式）
 * @param ip_addr PC 的 IP 地址，NULL 表示使用广播模式
 * @param port 端口号
 * @return ESP_OK 成功
 */
esp_err_t device_report_set_target(const char *ip_addr, uint16_t port);

/**
 * @brief 反初始化设备上报模块
 */
void device_report_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* DEVICE_REPORT_BSP_H */





