/**
 * @file device_report_bsp.c
 * @brief 设备信息上报模块实现
 */

#include "device_report_bsp.h"
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "cJSON.h"

static const char *TAG = "device_report";

/* 状态管理 */
static volatile device_report_state_t s_report_state = REPORT_STATE_IDLE;
static volatile bool s_report_running = false;

/* 配置 */
static char s_device_name[32] = "Skydimo";
static char s_device_type[16] = "SK-LCD";
static uint16_t s_broadcast_port = DEVICE_REPORT_DEFAULT_PORT;
static uint32_t s_report_interval_ms = DEVICE_REPORT_DEFAULT_INTERVAL;
static bool s_broadcast_enabled = true;
static device_report_state_cb_t s_state_cb = NULL;
static void *s_user_data = NULL;

/* 目标地址（单播模式） */
static char s_target_ip[16] = {0};
static uint16_t s_target_port = DEVICE_REPORT_DEFAULT_PORT;

/* 任务相关 */
static TaskHandle_t s_report_task_handle = NULL;
static int s_socket = -1;

/* 设备启动时间戳 */
static int64_t s_start_time_us = 0;

/******************** 内部函数 ********************/

static void set_state(device_report_state_t state)
{
    if (s_report_state != state) {
        s_report_state = state;
        ESP_LOGI(TAG, "State changed to: %d", state);
        if (s_state_cb) {
            s_state_cb(state, s_user_data);
        }
    }
}

/**
 * @brief 获取设备 IP 地址
 */
static esp_err_t get_device_ip(char *ip_buf, size_t buf_len)
{
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (!netif) {
        return ESP_ERR_NOT_FOUND;
    }
    
    esp_netif_ip_info_t ip_info;
    esp_err_t ret = esp_netif_get_ip_info(netif, &ip_info);
    if (ret != ESP_OK) {
        return ret;
    }
    
    snprintf(ip_buf, buf_len, IPSTR, IP2STR(&ip_info.ip));
    return ESP_OK;
}

/**
 * @brief 获取设备 MAC 地址
 */
static esp_err_t get_device_mac(char *mac_buf, size_t buf_len)
{
    uint8_t mac[6];
    esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, mac);
    if (ret != ESP_OK) {
        return ret;
    }
    
    snprintf(mac_buf, buf_len, "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return ESP_OK;
}

/**
 * @brief 获取 WiFi 信号强度
 */
static int8_t get_wifi_rssi(void)
{
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        return ap_info.rssi;
    }
    return 0;
}

/**
 * @brief 构建上报 JSON 数据
 */
static char* build_report_json(void)
{
    device_info_t info;
    
    /* 获取设备信息 */
    strncpy(info.device_name, s_device_name, sizeof(info.device_name) - 1);
    strncpy(info.device_type, s_device_type, sizeof(info.device_type) - 1);
    
    if (get_device_ip(info.ip_address, sizeof(info.ip_address)) != ESP_OK) {
        strcpy(info.ip_address, "0.0.0.0");
    }
    
    if (get_device_mac(info.mac_address, sizeof(info.mac_address)) != ESP_OK) {
        strcpy(info.mac_address, "00:00:00:00:00:00");
    }
    
    info.wifi_rssi = (uint8_t)(get_wifi_rssi() * -1);  /* 转为正数方便显示 */
    info.uptime_ms = (uint32_t)((esp_timer_get_time() - s_start_time_us) / 1000);
    info.free_heap_kb = (uint16_t)(esp_get_free_heap_size() / 1024);
    
    /* 构建 JSON */
    cJSON *root = cJSON_CreateObject();
    
    /* 消息类型标识 */
    cJSON_AddStringToObject(root, "type", "device_report");
    cJSON_AddStringToObject(root, "version", "1.0");
    
    /* 设备基本信息 */
    cJSON_AddStringToObject(root, "name", info.device_name);
    cJSON_AddStringToObject(root, "device_type", info.device_type);
    cJSON_AddStringToObject(root, "ip", info.ip_address);
    cJSON_AddStringToObject(root, "mac", info.mac_address);
    
    /* 状态信息 */
    cJSON_AddNumberToObject(root, "rssi", -(int)info.wifi_rssi);  /* 还原为负数 */
    cJSON_AddNumberToObject(root, "uptime_s", info.uptime_ms / 1000);
    cJSON_AddNumberToObject(root, "free_heap_kb", info.free_heap_kb);
    
    /* 时间戳 */
    cJSON_AddNumberToObject(root, "timestamp", (double)esp_timer_get_time() / 1000000.0);
    
    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    
    return json_str;
}

/**
 * @brief 关闭并重置 socket
 */
static void close_socket(void)
{
    if (s_socket >= 0) {
        close(s_socket);
        s_socket = -1;
    }
}

/**
 * @brief 创建 UDP socket
 */
static esp_err_t create_socket(void)
{
    /* 如果已有 socket，先关闭 */
    close_socket();
    
    s_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s_socket < 0) {
        ESP_LOGE(TAG, "Failed to create socket: %d", errno);
        return ESP_FAIL;
    }
    
    /* 设置广播权限 */
    int broadcast_enable = 1;
    if (setsockopt(s_socket, SOL_SOCKET, SO_BROADCAST, 
                   &broadcast_enable, sizeof(broadcast_enable)) < 0) {
        ESP_LOGW(TAG, "Failed to set SO_BROADCAST");
    }
    
    /* 设置发送超时 */
    struct timeval timeout = {.tv_sec = 1, .tv_usec = 0};
    setsockopt(s_socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    
    ESP_LOGI(TAG, "Socket created successfully");
    return ESP_OK;
}

/**
 * @brief 获取子网广播地址
 */
static esp_err_t get_subnet_broadcast_addr(struct in_addr *broadcast_addr)
{
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (!netif) {
        return ESP_ERR_NOT_FOUND;
    }
    
    esp_netif_ip_info_t ip_info;
    esp_err_t ret = esp_netif_get_ip_info(netif, &ip_info);
    if (ret != ESP_OK) {
        return ret;
    }
    
    /* 计算子网广播地址: IP | ~Netmask */
    broadcast_addr->s_addr = ip_info.ip.addr | ~ip_info.netmask.addr;
    return ESP_OK;
}

/**
 * @brief 发送 UDP 报文
 */
static esp_err_t send_udp_report(const char *json_data)
{
    if (!json_data) {
        return ESP_ERR_INVALID_ARG;
    }
    
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(s_target_port);
    
    /* 确定目标地址：单播或子网广播 */
    if (strlen(s_target_ip) > 0) {
        /* 单播模式 */
        inet_pton(AF_INET, s_target_ip, &dest_addr.sin_addr);
        ESP_LOGD(TAG, "Sending unicast to %s:%d", s_target_ip, s_target_port);
    } else {
        /* 子网广播模式 - 使用计算出的子网广播地址 */
        if (get_subnet_broadcast_addr(&dest_addr.sin_addr) != ESP_OK) {
            /* 回退到全局广播 */
            dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
            ESP_LOGW(TAG, "Using global broadcast (fallback)");
        } else {
            char broadcast_ip[16];
            inet_ntop(AF_INET, &dest_addr.sin_addr, broadcast_ip, sizeof(broadcast_ip));
            ESP_LOGI(TAG, "Using subnet broadcast: %s", broadcast_ip);
        }
    }
    
    /* 如果 socket 无效，尝试重新创建 */
    if (s_socket < 0) {
        if (create_socket() != ESP_OK) {
            return ESP_FAIL;
        }
    }
    
    /* 发送数据 */
    int len = strlen(json_data);
    int sent = sendto(s_socket, json_data, len, 0, 
                      (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    
    if (sent < 0) {
        ESP_LOGE(TAG, "Failed to send: %d", errno);
        /* 发送失败，关闭 socket 以便下次重建 */
        close_socket();
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Broadcast sent (%d bytes) to port %d", sent, s_target_port);
    return ESP_OK;
}

/**
 * @brief 等待设备获取有效 IP 地址
 */
static bool wait_for_valid_ip(int timeout_ms)
{
    char ip_buf[16];
    int waited = 0;
    const int check_interval = 100;
    
    while (waited < timeout_ms) {
        if (get_device_ip(ip_buf, sizeof(ip_buf)) == ESP_OK) {
            /* 检查是否是有效 IP（不是 0.0.0.0） */
            if (strcmp(ip_buf, "0.0.0.0") != 0) {
                ESP_LOGI(TAG, "Got valid IP: %s", ip_buf);
                return true;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(check_interval));
        waited += check_interval;
    }
    
    ESP_LOGW(TAG, "Timeout waiting for valid IP");
    return false;
}

/**
 * @brief 上报任务
 */
static void report_task(void *arg)
{
    ESP_LOGI(TAG, "Report task started, interval=%lu ms, port=%d", 
             (unsigned long)s_report_interval_ms, s_target_port);
    
    set_state(REPORT_STATE_RUNNING);
    
    /* 等待获取有效 IP 地址（最多等待 10 秒） */
    if (!wait_for_valid_ip(10000)) {
        ESP_LOGE(TAG, "Failed to get valid IP, stopping report task");
        set_state(REPORT_STATE_ERROR);
        s_report_running = false;
        s_report_task_handle = NULL;
        vTaskDelete(NULL);
        return;
    }
    
    /* 获取到 IP 后，再等待 500ms 确保网络完全就绪 */
    vTaskDelay(pdMS_TO_TICKS(500));
    
    /* 立即发送一次 */
    char *json = build_report_json();
    if (json) {
        ESP_LOGI(TAG, "First report: %s", json);
        esp_err_t ret = send_udp_report(json);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "First report sent successfully!");
        } else {
            ESP_LOGW(TAG, "First report failed, will retry");
        }
        free(json);
    }
    
    while (s_report_running) {
        vTaskDelay(pdMS_TO_TICKS(s_report_interval_ms));
        
        if (!s_report_running) {
            break;
        }
        
        json = build_report_json();
        if (json) {
            esp_err_t ret = send_udp_report(json);
            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "Report failed, will retry next interval");
            } else {
                ESP_LOGI(TAG, "Report sent: %s", json);
            }
            free(json);
        }
    }
    
    /* 清理 socket */
    close_socket();
    
    set_state(REPORT_STATE_IDLE);
    ESP_LOGI(TAG, "Report task stopped");
    
    s_report_task_handle = NULL;
    vTaskDelete(NULL);
}

/******************** 公共函数实现 ********************/

esp_err_t device_report_init(const device_report_config_t *config)
{
    /* 记录启动时间 */
    if (s_start_time_us == 0) {
        s_start_time_us = esp_timer_get_time();
    }
    
    /* 应用配置 */
    if (config) {
        if (config->device_name && strlen(config->device_name) > 0) {
            strncpy(s_device_name, config->device_name, sizeof(s_device_name) - 1);
        }
        
        if (config->device_type && strlen(config->device_type) > 0) {
            strncpy(s_device_type, config->device_type, sizeof(s_device_type) - 1);
        }
        
        s_broadcast_port = config->broadcast_port > 0 ? 
                           config->broadcast_port : DEVICE_REPORT_DEFAULT_PORT;
        s_target_port = s_broadcast_port;
        
        s_report_interval_ms = config->report_interval_ms > 0 ? 
                               config->report_interval_ms : DEVICE_REPORT_DEFAULT_INTERVAL;
        
        s_broadcast_enabled = config->broadcast_enabled;
        s_state_cb = config->state_cb;
        s_user_data = config->user_data;
    }
    
    ESP_LOGI(TAG, "Device report initialized: name=%s, type=%s, port=%d, interval=%lu ms",
             s_device_name, s_device_type, s_target_port, (unsigned long)s_report_interval_ms);
    
    return ESP_OK;
}

esp_err_t device_report_start(void)
{
    if (s_report_running) {
        ESP_LOGW(TAG, "Report already running");
        return ESP_OK;
    }
    
    s_report_running = true;
    
    /* 创建上报任务 */
    BaseType_t ret = xTaskCreate(report_task, "dev_report", 4096, NULL, 3, &s_report_task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create report task");
        s_report_running = false;
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Device report started");
    return ESP_OK;
}

esp_err_t device_report_stop(void)
{
    if (!s_report_running) {
        return ESP_OK;
    }
    
    s_report_running = false;
    
    /* 等待任务结束 */
    int timeout = 20;  /* 最多等待 2 秒 */
    while (s_report_task_handle && timeout > 0) {
        vTaskDelay(pdMS_TO_TICKS(100));
        timeout--;
    }
    
    ESP_LOGI(TAG, "Device report stopped");
    return ESP_OK;
}

esp_err_t device_report_send_now(void)
{
    char *json = build_report_json();
    if (!json) {
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "Sending immediate report: %s", json);
    esp_err_t ret = send_udp_report(json);
    free(json);
    
    return ret;
}

device_report_state_t device_report_get_state(void)
{
    return s_report_state;
}

esp_err_t device_report_get_info(device_info_t *info)
{
    if (!info) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memset(info, 0, sizeof(device_info_t));
    
    strncpy(info->device_name, s_device_name, sizeof(info->device_name) - 1);
    strncpy(info->device_type, s_device_type, sizeof(info->device_type) - 1);
    
    get_device_ip(info->ip_address, sizeof(info->ip_address));
    get_device_mac(info->mac_address, sizeof(info->mac_address));
    
    info->wifi_rssi = (uint8_t)(get_wifi_rssi() * -1);
    info->uptime_ms = (uint32_t)((esp_timer_get_time() - s_start_time_us) / 1000);
    info->free_heap_kb = (uint16_t)(esp_get_free_heap_size() / 1024);
    
    return ESP_OK;
}

esp_err_t device_report_set_interval(uint32_t interval_ms)
{
    if (interval_ms < 100) {
        return ESP_ERR_INVALID_ARG;
    }
    
    s_report_interval_ms = interval_ms;
    ESP_LOGI(TAG, "Report interval set to %lu ms", (unsigned long)interval_ms);
    return ESP_OK;
}

esp_err_t device_report_set_target(const char *ip_addr, uint16_t port)
{
    if (ip_addr && strlen(ip_addr) > 0) {
        strncpy(s_target_ip, ip_addr, sizeof(s_target_ip) - 1);
        ESP_LOGI(TAG, "Target set to unicast: %s", s_target_ip);
    } else {
        s_target_ip[0] = '\0';
        ESP_LOGI(TAG, "Target set to broadcast");
    }
    
    if (port > 0) {
        s_target_port = port;
    }
    
    return ESP_OK;
}

void device_report_deinit(void)
{
    device_report_stop();
    
    s_device_name[0] = '\0';
    s_device_type[0] = '\0';
    s_target_ip[0] = '\0';
    s_state_cb = NULL;
    s_user_data = NULL;
    
    ESP_LOGI(TAG, "Device report deinitialized");
}

