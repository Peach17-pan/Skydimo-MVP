/**
 * @file wifi_provision_bsp.c
 * @brief WiFi 配网模块实现
 */

#include "wifi_provision_bsp.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG = "wifi_prov";

/* NVS 存储相关 */
#define NVS_NAMESPACE       "wifi_config"
#define NVS_KEY_SSID        "ssid"
#define NVS_KEY_PASSWORD    "password"

/* 默认 AP 配置 */
#define DEFAULT_AP_SSID         "Skydimo_Config"
#define DEFAULT_AP_PASSWORD     "12345678"
#define DEFAULT_AP_CHANNEL      1
#define DEFAULT_MAX_STA_CONN    4

/* 状态管理 */
static volatile wifi_prov_state_t s_prov_state = WIFI_PROV_STATE_IDLE;
static volatile bool s_prov_active = false;
static volatile uint8_t s_connected_sta_count = 0;

/* 配置 */
static char s_ap_ssid[33] = {0};
static char s_ap_password[65] = {0};
static char s_ap_ip[16] = "192.168.4.1";
static wifi_prov_state_cb_t s_state_cb = NULL;
static void *s_user_data = NULL;

/* 网络接口 */
static esp_netif_t *s_ap_netif = NULL;
static bool s_wifi_initialized = false;

/******************** 内部函数 ********************/

static void set_state(wifi_prov_state_t state)
{
    if (s_prov_state != state) {
        s_prov_state = state;
        ESP_LOGI(TAG, "State changed to: %d", state);
        if (s_state_cb) {
            s_state_cb(state, s_user_data);
        }
    }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_AP_START:
                ESP_LOGI(TAG, "AP started");
                set_state(WIFI_PROV_STATE_AP_STARTED);
                break;
                
            case WIFI_EVENT_AP_STOP:
                ESP_LOGI(TAG, "AP stopped");
                set_state(WIFI_PROV_STATE_IDLE);
                break;
                
            case WIFI_EVENT_AP_STACONNECTED: {
                wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
                ESP_LOGI(TAG, "Station connected: MAC=" MACSTR ", AID=%d",
                         MAC2STR(event->mac), event->aid);
                s_connected_sta_count++;
                set_state(WIFI_PROV_STATE_STA_CONNECTED);
                break;
            }
            
            case WIFI_EVENT_AP_STADISCONNECTED: {
                wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
                ESP_LOGI(TAG, "Station disconnected: MAC=" MACSTR ", AID=%d",
                         MAC2STR(event->mac), event->aid);
                if (s_connected_sta_count > 0) {
                    s_connected_sta_count--;
                }
                if (s_connected_sta_count == 0 && s_prov_active) {
                    set_state(WIFI_PROV_STATE_AP_STARTED);
                }
                break;
            }
            
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "STA started");
                break;
                
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "STA connected to AP");
                break;
                
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGW(TAG, "STA disconnected from AP");
                if (s_prov_state == WIFI_PROV_STATE_CONNECTING) {
                    set_state(WIFI_PROV_STATE_FAILED);
                }
                break;
                
            default:
                break;
        }
    } else if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_AP_STAIPASSIGNED: {
                ip_event_ap_staipassigned_t *event = (ip_event_ap_staipassigned_t *)event_data;
                ESP_LOGI(TAG, "Assigned IP to station: " IPSTR, IP2STR(&event->ip));
                break;
            }
            
            case IP_EVENT_STA_GOT_IP: {
                ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
                ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
                set_state(WIFI_PROV_STATE_CONNECTED);
                break;
            }
            
            default:
                break;
        }
    }
}

static esp_err_t init_nvs(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition truncated, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}

static esp_err_t save_wifi_config(const char *ssid, const char *password)
{
    nvs_handle_t handle;
    esp_err_t ret;
    
    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = nvs_set_str(handle, NVS_KEY_SSID, ssid);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save SSID: %s", esp_err_to_name(ret));
        nvs_close(handle);
        return ret;
    }
    
    ret = nvs_set_str(handle, NVS_KEY_PASSWORD, password);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save password: %s", esp_err_to_name(ret));
        nvs_close(handle);
        return ret;
    }
    
    ret = nvs_commit(handle);
    nvs_close(handle);
    
    ESP_LOGI(TAG, "WiFi config saved: SSID=%s", ssid);
    return ret;
}

/******************** 公共函数实现 ********************/

esp_err_t wifi_provision_init(const wifi_prov_config_t *config)
{
    esp_err_t ret;
    
    /* 初始化 NVS */
    ret = init_nvs();
    if (ret != ESP_OK) {
        return ret;
    }
    
    /* 保存配置 */
    if (config) {
        if (config->ap_ssid) {
            strncpy(s_ap_ssid, config->ap_ssid, sizeof(s_ap_ssid) - 1);
        } else {
            strncpy(s_ap_ssid, DEFAULT_AP_SSID, sizeof(s_ap_ssid) - 1);
        }
        
        if (config->ap_password) {
            strncpy(s_ap_password, config->ap_password, sizeof(s_ap_password) - 1);
        } else {
            strncpy(s_ap_password, DEFAULT_AP_PASSWORD, sizeof(s_ap_password) - 1);
        }
        
        s_state_cb = config->state_cb;
        s_user_data = config->user_data;
    } else {
        strncpy(s_ap_ssid, DEFAULT_AP_SSID, sizeof(s_ap_ssid) - 1);
        strncpy(s_ap_password, DEFAULT_AP_PASSWORD, sizeof(s_ap_password) - 1);
    }
    
    /* 初始化网络接口 */
    if (!s_wifi_initialized) {
        ret = esp_netif_init();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to init netif: %s", esp_err_to_name(ret));
            return ret;
        }
        
        ret = esp_event_loop_create_default();
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "Failed to create event loop: %s", esp_err_to_name(ret));
            return ret;
        }
        
        /* 初始化 WiFi */
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ret = esp_wifi_init(&cfg);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to init wifi: %s", esp_err_to_name(ret));
            return ret;
        }
        
        /* 注册事件处理器 */
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                            &wifi_event_handler, NULL, NULL));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID,
                                                            &wifi_event_handler, NULL, NULL));
        
        s_wifi_initialized = true;
    }
    
    ESP_LOGI(TAG, "WiFi provision initialized, AP SSID: %s", s_ap_ssid);
    return ESP_OK;
}

esp_err_t wifi_provision_start(void)
{
    esp_err_t ret;
    
    if (s_prov_active) {
        ESP_LOGW(TAG, "Provision already active");
        return ESP_OK;
    }
    
    /* 停止现有 WiFi 连接 */
    esp_wifi_stop();
    
    /* 创建 AP 网络接口 */
    if (!s_ap_netif) {
        s_ap_netif = esp_netif_create_default_wifi_ap();
        if (!s_ap_netif) {
            ESP_LOGE(TAG, "Failed to create AP netif");
            return ESP_FAIL;
        }
    }
    
    /* 配置 AP */
    wifi_config_t wifi_config = {
        .ap = {
            .channel = DEFAULT_AP_CHANNEL,
            .max_connection = DEFAULT_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = false,
            },
        },
    };
    
    strncpy((char *)wifi_config.ap.ssid, s_ap_ssid, sizeof(wifi_config.ap.ssid));
    wifi_config.ap.ssid_len = strlen(s_ap_ssid);
    
    if (strlen(s_ap_password) < 8) {
        /* 密码少于 8 位则设置为开放网络 */
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        wifi_config.ap.password[0] = '\0';
    } else {
        strncpy((char *)wifi_config.ap.password, s_ap_password, sizeof(wifi_config.ap.password));
    }
    
    /* 设置 AP 模式并启动 */
    ret = esp_wifi_set_mode(WIFI_MODE_AP);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set AP mode: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set AP config: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start AP: %s", esp_err_to_name(ret));
        return ret;
    }
    
    s_prov_active = true;
    s_connected_sta_count = 0;
    
    ESP_LOGI(TAG, "SoftAP started - SSID: %s, Password: %s", 
             s_ap_ssid, strlen(s_ap_password) >= 8 ? s_ap_password : "(Open)");
    ESP_LOGI(TAG, "Connect to WiFi and access http://%s to configure", s_ap_ip);
    
    return ESP_OK;
}

esp_err_t wifi_provision_stop(void)
{
    if (!s_prov_active) {
        return ESP_OK;
    }
    
    esp_err_t ret = esp_wifi_stop();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop WiFi: %s", esp_err_to_name(ret));
        return ret;
    }
    
    s_prov_active = false;
    s_connected_sta_count = 0;
    set_state(WIFI_PROV_STATE_IDLE);
    
    ESP_LOGI(TAG, "Provision stopped");
    return ESP_OK;
}

wifi_prov_state_t wifi_provision_get_state(void)
{
    return s_prov_state;
}

bool wifi_provision_is_active(void)
{
    return s_prov_active;
}

const char* wifi_provision_get_ap_ssid(void)
{
    return s_ap_ssid;
}

const char* wifi_provision_get_ap_ip(void)
{
    return s_ap_ip;
}

uint8_t wifi_provision_get_sta_count(void)
{
    return s_connected_sta_count;
}

esp_err_t wifi_provision_connect_sta(const char *ssid, const char *password)
{
    if (!ssid || strlen(ssid) == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    /* 保存配置到 NVS */
    esp_err_t ret = save_wifi_config(ssid, password ? password : "");
    if (ret != ESP_OK) {
        return ret;
    }
    
    /* 停止 AP */
    wifi_provision_stop();
    
    /* 创建 STA 网络接口 */
    esp_netif_create_default_wifi_sta();
    
    /* 配置 STA */
    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    if (password) {
        strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    }
    
    /* 设置 STA 模式 */
    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) {
        return ret;
    }
    
    ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK) {
        return ret;
    }
    
    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        return ret;
    }
    
    set_state(WIFI_PROV_STATE_CONNECTING);
    
    ret = esp_wifi_connect();
    if (ret != ESP_OK) {
        set_state(WIFI_PROV_STATE_FAILED);
        return ret;
    }
    
    ESP_LOGI(TAG, "Connecting to WiFi: %s", ssid);
    return ESP_OK;
}

esp_err_t wifi_provision_get_saved_ssid(char *ssid, size_t max_len)
{
    nvs_handle_t handle;
    esp_err_t ret;
    
    ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        return ret;
    }
    
    size_t required_size = max_len;
    ret = nvs_get_str(handle, NVS_KEY_SSID, ssid, &required_size);
    nvs_close(handle);
    
    return ret;
}

bool wifi_provision_has_saved_config(void)
{
    nvs_handle_t handle;
    esp_err_t ret;
    
    ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        return false;
    }
    
    size_t required_size = 0;
    ret = nvs_get_str(handle, NVS_KEY_SSID, NULL, &required_size);
    nvs_close(handle);
    
    return (ret == ESP_OK && required_size > 1);
}



