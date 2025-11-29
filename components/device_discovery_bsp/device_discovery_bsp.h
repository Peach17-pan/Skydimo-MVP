/**
 * @file device_discovery_bsp.h
 * @brief 设备发现模块头文件
 * 
 * 提供多种设备发现机制：
 * - mDNS 服务发现（推荐）
 * - UDP 广播
 * - TCP 心跳（可选）
 */

#ifndef DEVICE_DISCOVERY_BSP_H
#define DEVICE_DISCOVERY_BSP_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 默认配置 */
#define DISCOVERY_DEFAULT_PORT          9527    /* 默认端口 */
#define DISCOVERY_DEFAULT_INTERVAL_MS   5000    /* 默认广播间隔 (ms) */
#define DISCOVERY_DEFAULT_HOSTNAME      "skydimo"
#define DISCOVERY_DEFAULT_INSTANCE      "Skydimo-Device"
#define DISCOVERY_DEFAULT_SERVICE_TYPE  "_skydimo"
#define DISCOVERY_DEFAULT_PROTO         "_tcp"

/**
 * @brief 发现模式枚举
 */
typedef enum {
    DISCOVERY_MODE_NONE       = 0x00,   /* 不启用任何发现模式 */
    DISCOVERY_MODE_MDNS       = 0x01,   /* mDNS 服务发现 */
    DISCOVERY_MODE_UDP_BCAST  = 0x02,   /* UDP 广播 */
    DISCOVERY_MODE_TCP_HB     = 0x04,   /* TCP 心跳 */
    DISCOVERY_MODE_ALL        = 0x07,   /* 启用全部模式 */
} device_discovery_mode_t;

/**
 * @brief 发现状态枚举
 */
typedef enum {
    DISCOVERY_STATE_IDLE = 0,       /* 空闲状态 */
    DISCOVERY_STATE_RUNNING,        /* 正在运行 */
    DISCOVERY_STATE_ERROR,          /* 错误状态 */
} device_discovery_state_t;

/**
 * @brief 状态回调函数类型
 */
typedef void (*device_discovery_state_cb_t)(device_discovery_state_t state, void *user_data);

/**
 * @brief mDNS 配置结构体
 */
typedef struct {
    const char *hostname;           /* 主机名，如 "skydimo" -> skydimo.local */
    const char *instance_name;      /* 实例名称，如 "Skydimo-Device" */
    const char *service_type;       /* 服务类型，如 "_skydimo" */
    const char *proto;              /* 协议类型，如 "_tcp" */
    uint16_t port;                  /* 服务端口 */
} device_discovery_mdns_config_t;

/**
 * @brief UDP 广播配置结构体
 */
typedef struct {
    uint16_t port;                  /* 广播端口 */
    uint32_t interval_ms;           /* 广播间隔 (ms) */
} device_discovery_udp_config_t;

/**
 * @brief TCP 心跳配置结构体
 */
typedef struct {
    const char *server_ip;          /* 服务器 IP */
    uint16_t server_port;           /* 服务器端口 */
    uint32_t interval_ms;           /* 心跳间隔 (ms) */
    uint32_t timeout_ms;            /* 超时时间 (ms) */
} device_discovery_tcp_config_t;

/**
 * @brief 设备发现配置结构体
 */
typedef struct {
    const char *device_name;                /* 设备名称 */
    const char *device_type;                /* 设备类型 */
    device_discovery_mode_t mode;           /* 启用的发现模式 */
    device_discovery_mdns_config_t mdns;    /* mDNS 配置 */
    device_discovery_udp_config_t udp;      /* UDP 配置 */
    device_discovery_tcp_config_t tcp;      /* TCP 配置 */
    device_discovery_state_cb_t state_cb;   /* 状态回调函数 */
    void *user_data;                        /* 用户数据 */
} device_discovery_config_t;

/**
 * @brief 设备信息结构体
 */
typedef struct {
    char device_name[32];       /* 设备名称 */
    char device_type[16];       /* 设备类型 */
    char hostname[32];          /* mDNS 主机名 */
    char ip_address[16];        /* IP 地址 */
    char mac_address[18];       /* MAC 地址 */
    int8_t wifi_rssi;           /* WiFi 信号强度 */
    uint32_t uptime_s;          /* 运行时间 (秒) */
    uint16_t free_heap_kb;      /* 剩余堆内存 (KB) */
} device_discovery_info_t;

/**
 * @brief 默认配置宏
 */
#define DEVICE_DISCOVERY_DEFAULT_CONFIG() { \
    .device_name = "Skydimo", \
    .device_type = "SK-LCD", \
    .mode = DISCOVERY_MODE_MDNS, \
    .mdns = { \
        .hostname = DISCOVERY_DEFAULT_HOSTNAME, \
        .instance_name = DISCOVERY_DEFAULT_INSTANCE, \
        .service_type = DISCOVERY_DEFAULT_SERVICE_TYPE, \
        .proto = DISCOVERY_DEFAULT_PROTO, \
        .port = DISCOVERY_DEFAULT_PORT, \
    }, \
    .udp = { \
        .port = DISCOVERY_DEFAULT_PORT, \
        .interval_ms = DISCOVERY_DEFAULT_INTERVAL_MS, \
    }, \
    .tcp = { \
        .server_ip = NULL, \
        .server_port = DISCOVERY_DEFAULT_PORT, \
        .interval_ms = DISCOVERY_DEFAULT_INTERVAL_MS, \
        .timeout_ms = 3000, \
    }, \
    .state_cb = NULL, \
    .user_data = NULL, \
}

/**
 * @brief 初始化设备发现模块
 * @param config 配置参数，NULL 使用默认配置
 * @return ESP_OK 成功
 */
esp_err_t device_discovery_init(const device_discovery_config_t *config);

/**
 * @brief 启动设备发现服务
 * @return ESP_OK 成功
 */
esp_err_t device_discovery_start(void);

/**
 * @brief 停止设备发现服务
 * @return ESP_OK 成功
 */
esp_err_t device_discovery_stop(void);

/**
 * @brief 获取当前状态
 * @return 当前状态
 */
device_discovery_state_t device_discovery_get_state(void);

/**
 * @brief 获取设备信息
 * @param info 输出设备信息
 * @return ESP_OK 成功
 */
esp_err_t device_discovery_get_info(device_discovery_info_t *info);

/**
 * @brief 获取 mDNS 主机名（如 "skydimo.local"）
 * @return 主机名字符串
 */
const char* device_discovery_get_hostname(void);

/**
 * @brief 设置自定义 mDNS TXT 记录
 * @param key 键
 * @param value 值
 * @return ESP_OK 成功
 */
esp_err_t device_discovery_set_txt_record(const char *key, const char *value);

/**
 * @brief 立即发送一次广播（仅 UDP 模式）
 * @return ESP_OK 成功
 */
esp_err_t device_discovery_broadcast_now(void);

/**
 * @brief 更新设备名称
 * @param name 新的设备名称
 * @return ESP_OK 成功
 */
esp_err_t device_discovery_set_device_name(const char *name);

/**
 * @brief 反初始化设备发现模块
 */
void device_discovery_deinit(void);

/**
 * @brief 检查 mDNS 是否正在运行
 * @return true 正在运行
 */
bool device_discovery_is_mdns_running(void);

/**
 * @brief 检查 UDP 广播是否正在运行
 * @return true 正在运行
 */
bool device_discovery_is_udp_running(void);

#ifdef __cplusplus
}
#endif

#endif /* DEVICE_DISCOVERY_BSP_H */

