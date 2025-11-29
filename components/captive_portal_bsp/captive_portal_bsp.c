/**
 * @file captive_portal_bsp.c
 * @brief Captive Portal Web 配网模块实现
 */

#include "captive_portal_bsp.h"
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "cJSON.h"

static const char *TAG = "captive_portal";

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
static volatile captive_portal_state_t s_portal_state = PORTAL_STATE_IDLE;
static volatile bool s_portal_active = false;
static volatile uint8_t s_connected_sta_count = 0;

/* WiFi 连接重试 */
#define WIFI_CONNECT_RETRY_MAX  3
static volatile uint8_t s_wifi_retry_count = 0;

/* 配置 */
static char s_ap_ssid[33] = {0};
static char s_ap_password[65] = {0};
static char s_ap_ip[16] = "192.168.4.1";
static uint8_t s_ap_channel = DEFAULT_AP_CHANNEL;
static captive_portal_state_cb_t s_state_cb = NULL;
static void *s_user_data = NULL;

/* 网络接口 */
static esp_netif_t *s_ap_netif = NULL;
static esp_netif_t *s_sta_netif = NULL;
static bool s_wifi_initialized = false;

/* HTTP 服务器 */
static httpd_handle_t s_http_server = NULL;

/* 事件组 */
static EventGroupHandle_t s_wifi_event_group = NULL;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define WIFI_SCAN_DONE_BIT BIT2

/* WiFi 扫描结果缓存 */
#define MAX_SCAN_RESULTS 20
static wifi_scan_result_t s_scan_results[MAX_SCAN_RESULTS];
static uint16_t s_scan_count = 0;

/******************** HTML 页面内容 ********************/

static const char HTML_PAGE[] = 
"<!DOCTYPE html>"
"<html><head>"
"<meta charset=\"UTF-8\">"
"<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">"
"<title>Skydimo WiFi</title>"
"<style>"
"*{box-sizing:border-box;margin:0;padding:0}"
"body{font-family:-apple-system,sans-serif;background:linear-gradient(135deg,#1A1A2E,#16213E,#0F3460);min-height:100vh;color:#EEE;padding:20px}"
".c{max-width:380px;margin:0 auto;background:rgba(22,33,62,0.95);border-radius:16px;padding:25px;box-shadow:0 8px 32px rgba(0,0,0,0.3)}"
".logo{text-align:center;margin-bottom:20px}"
".logo h1{color:#E94560;font-size:24px}"
".logo p{color:#94A3B8;font-size:13px}"
".icon{width:50px;height:50px;margin:0 auto 10px;background:linear-gradient(135deg,#E94560,#0F3460);border-radius:50%;display:flex;align-items:center;justify-content:center;font-size:24px}"
".form-group{margin-bottom:15px}"
"label{display:block;margin-bottom:6px;color:#94A3B8;font-size:13px}"
"input,select{width:100%;padding:12px;border:2px solid #0F3460;border-radius:8px;background:#1A1A2E;color:#EEE;font-size:15px}"
"input:focus{outline:none;border-color:#E94560}"
".btn{width:100%;padding:12px;border:none;border-radius:8px;font-size:15px;font-weight:600;cursor:pointer;margin-bottom:8px}"
".btn-p{background:linear-gradient(135deg,#E94560,#d13652);color:#fff}"
".btn-s{background:#0F3460;color:#EEE}"
".wifi-list{max-height:180px;overflow-y:auto;border-radius:8px;border:2px solid #0F3460;margin-bottom:15px}"
".wifi-item{padding:10px 12px;border-bottom:1px solid #0F3460;cursor:pointer;display:flex;justify-content:space-between}"
".wifi-item:hover{background:rgba(15,52,96,0.5)}"
".wifi-item.sel{background:rgba(233,69,96,0.2);border-left:3px solid #E94560}"
".sig{font-size:11px}"
".sig.g{color:#10B981}.sig.m{color:#F59E0B}.sig.b{color:#EF4444}"
".st{text-align:center;padding:12px;border-radius:8px;margin-top:15px;font-weight:500}"
".st.ok{background:rgba(16,185,129,0.2);color:#10B981}"
".st.err{background:rgba(239,68,68,0.2);color:#EF4444}"
".st.info{background:rgba(245,158,11,0.2);color:#F59E0B}"
".ld{display:inline-block;width:16px;height:16px;border:2px solid #E94560;border-radius:50%;border-top-color:transparent;animation:sp 1s linear infinite;margin-right:8px}"
"@keyframes sp{to{transform:rotate(360deg)}}"
".hide{display:none}"
"</style></head><body>"
"<div class=\"c\">"
"<div class=\"logo\"><div class=\"icon\">&#128246;</div><h1>Skydimo</h1><p>WiFi Configuration</p></div>"
"<form id=\"f\">"
"<div class=\"form-group\"><label>Available Networks</label>"
"<div class=\"wifi-list\" id=\"wl\"><div class=\"wifi-item\" style=\"justify-content:center;color:#94A3B8\"><span class=\"ld\"></span>Scanning...</div></div></div>"
"<div class=\"form-group\"><label>WiFi Name</label><input type=\"text\" id=\"ssid\" placeholder=\"Enter SSID\" required></div>"
"<div class=\"form-group\"><label>Password</label><input type=\"password\" id=\"pwd\" placeholder=\"Enter password\"></div>"
"<button type=\"button\" class=\"btn btn-s\" onclick=\"scan()\">&#128260; Refresh</button>"
"<button type=\"submit\" class=\"btn btn-p\">&#128225; Connect</button>"
"</form>"
"<div id=\"st\" class=\"hide\"></div>"
"</div>"
"<script>"
"function sig(r){if(r>=-50)return['g','\\u2582\\u2584\\u2586\\u2588'];if(r>=-65)return['m','\\u2582\\u2584\\u2586_'];if(r>=-75)return['m','\\u2582\\u2584__'];return['b','\\u2582___']}"
"function scan(){var l=document.getElementById('wl');l.innerHTML='<div class=\"wifi-item\" style=\"justify-content:center;color:#94A3B8\"><span class=\"ld\"></span>Scanning...</div>';"
"fetch('/scan').then(r=>r.json()).then(d=>{l.innerHTML='';if(d.networks&&d.networks.length>0){d.networks.forEach(n=>{var s=sig(n.rssi);var i=document.createElement('div');i.className='wifi-item';i.innerHTML='<span>'+n.ssid+'</span><span class=\"sig '+s[0]+'\">'+s[1]+' '+n.rssi+'dBm</span>';i.onclick=function(){document.querySelectorAll('.wifi-item').forEach(e=>e.classList.remove('sel'));this.classList.add('sel');document.getElementById('ssid').value=n.ssid};l.appendChild(i)})}else{l.innerHTML='<div class=\"wifi-item\" style=\"justify-content:center;color:#94A3B8\">No networks found</div>'}}).catch(()=>{l.innerHTML='<div class=\"wifi-item\" style=\"justify-content:center;color:#EF4444\">Scan failed</div>'})}"
"document.getElementById('f').onsubmit=function(e){e.preventDefault();var ssid=document.getElementById('ssid').value;var pwd=document.getElementById('pwd').value;var st=document.getElementById('st');"
"if(!ssid){st.className='st err';st.innerHTML='Please enter SSID';return}"
"st.className='st info';st.innerHTML='<span class=\"ld\"></span>Connecting...';"
"fetch('/connect',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid:ssid,password:pwd})}).then(r=>r.json()).then(d=>{if(d.success){st.className='st ok';st.innerHTML='\\u2713 Connected!'}else{st.className='st err';st.innerHTML='\\u2717 Failed: '+(d.message||'Error')}}).catch(()=>{st.className='st info';st.innerHTML='Connecting... Device will restart.'})};"
"scan();"
"</script></body></html>";

/******************** 内部函数 ********************/

static void set_state(captive_portal_state_t state)
{
    if (s_portal_state != state) {
        s_portal_state = state;
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
                set_state(PORTAL_STATE_AP_STARTED);
                break;
                
            case WIFI_EVENT_AP_STOP:
                ESP_LOGI(TAG, "AP stopped");
                /* 不要在连接过程中或已连接时重置状态 */
                if (s_portal_state != PORTAL_STATE_CONNECTED && 
                    s_portal_state != PORTAL_STATE_CONNECTING) {
                    set_state(PORTAL_STATE_IDLE);
                }
                break;
                
            case WIFI_EVENT_AP_STACONNECTED: {
                wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
                ESP_LOGI(TAG, "Station connected: MAC=" MACSTR, MAC2STR(event->mac));
                s_connected_sta_count++;
                set_state(PORTAL_STATE_STA_CONNECTED);
                break;
            }
            
            case WIFI_EVENT_AP_STADISCONNECTED: {
                wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
                ESP_LOGI(TAG, "Station disconnected: MAC=" MACSTR, MAC2STR(event->mac));
                if (s_connected_sta_count > 0) {
                    s_connected_sta_count--;
                }
                break;
            }
            
            case WIFI_EVENT_SCAN_DONE:
                ESP_LOGI(TAG, "WiFi scan done");
                if (s_wifi_event_group) {
                    xEventGroupSetBits(s_wifi_event_group, WIFI_SCAN_DONE_BIT);
                }
                break;
                
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "STA started");
                /* 如果处于连接状态，自动开始连接 */
                if (s_portal_state == PORTAL_STATE_CONNECTING) {
                    ESP_LOGI(TAG, "Starting WiFi connection...");
                    esp_wifi_connect();
                }
                break;
                
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "STA connected to AP");
                break;
                
            case WIFI_EVENT_STA_DISCONNECTED: {
                wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data;
                ESP_LOGW(TAG, "STA disconnected from AP, reason: %d", event->reason);
                /* 常见原因码:
                 * 2 = AUTH_EXPIRE, 4 = ASSOC_EXPIRE, 15 = 4WAY_HANDSHAKE_TIMEOUT
                 * 201 = NO_AP_FOUND, 202 = AUTH_FAIL, 203 = ASSOC_FAIL
                 */
                if (s_portal_state == PORTAL_STATE_CONNECTING) {
                    if (s_wifi_retry_count < WIFI_CONNECT_RETRY_MAX) {
                        s_wifi_retry_count++;
                        ESP_LOGI(TAG, "Retrying WiFi connection... (%d/%d)", 
                                 s_wifi_retry_count, WIFI_CONNECT_RETRY_MAX);
                        vTaskDelay(pdMS_TO_TICKS(1000));  /* 等待 1 秒再重试 */
                        esp_wifi_connect();
                    } else {
                        ESP_LOGE(TAG, "WiFi connection failed after %d retries, last reason: %d", 
                                 WIFI_CONNECT_RETRY_MAX, event->reason);
                        s_wifi_retry_count = 0;
                        set_state(PORTAL_STATE_FAILED);
                        if (s_wifi_event_group) {
                            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
                        }
                    }
                }
                break;
            }
                
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
                s_wifi_retry_count = 0;  /* 重置重试计数 */
                set_state(PORTAL_STATE_CONNECTED);
                if (s_wifi_event_group) {
                    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
                }
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
    
    ret = nvs_set_str(handle, NVS_KEY_PASSWORD, password ? password : "");
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

static esp_err_t load_wifi_config(char *ssid, size_t ssid_len, char *password, size_t password_len)
{
    nvs_handle_t handle;
    esp_err_t ret;
    
    ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        return ret;
    }
    
    size_t required_size = ssid_len;
    ret = nvs_get_str(handle, NVS_KEY_SSID, ssid, &required_size);
    if (ret != ESP_OK) {
        nvs_close(handle);
        return ret;
    }
    
    required_size = password_len;
    ret = nvs_get_str(handle, NVS_KEY_PASSWORD, password, &required_size);
    nvs_close(handle);
    
    return ret;
}

/******************** HTTP 处理函数 ********************/

/* 主页面 */
static esp_err_t http_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "HTTP GET: %s", req->uri);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, HTML_PAGE, strlen(HTML_PAGE));
    return ESP_OK;
}

/* WiFi 扫描 API */
static esp_err_t http_scan_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "HTTP GET: /scan");
    
    /* 执行扫描 */
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = 100,
        .scan_time.active.max = 300,
    };
    
    esp_err_t ret = esp_wifi_scan_start(&scan_config, true);
    if (ret != ESP_OK) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"networks\":[],\"error\":\"Scan failed\"}");
        return ESP_OK;
    }
    
    /* 获取扫描结果 */
    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);
    
    wifi_ap_record_t *ap_records = NULL;
    if (ap_count > 0) {
        if (ap_count > MAX_SCAN_RESULTS) {
            ap_count = MAX_SCAN_RESULTS;
        }
        ap_records = malloc(sizeof(wifi_ap_record_t) * ap_count);
        if (ap_records) {
            esp_wifi_scan_get_ap_records(&ap_count, ap_records);
        }
    }
    
    /* 构建 JSON 响应 */
    cJSON *root = cJSON_CreateObject();
    cJSON *networks = cJSON_CreateArray();
    
    if (ap_records) {
        for (int i = 0; i < ap_count; i++) {
            /* 跳过空 SSID */
            if (strlen((char *)ap_records[i].ssid) == 0) continue;
            
            cJSON *net = cJSON_CreateObject();
            cJSON_AddStringToObject(net, "ssid", (char *)ap_records[i].ssid);
            cJSON_AddNumberToObject(net, "rssi", ap_records[i].rssi);
            cJSON_AddNumberToObject(net, "auth", ap_records[i].authmode);
            cJSON_AddItemToArray(networks, net);
        }
        free(ap_records);
    }
    
    cJSON_AddItemToObject(root, "networks", networks);
    
    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json_str);
    free(json_str);
    
    return ESP_OK;
}

/* 连接 API */
static esp_err_t http_connect_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "HTTP POST: /connect");
    
    /* 读取请求体 */
    char buf[256] = {0};
    int received = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (received <= 0) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"success\":false,\"message\":\"No data received\"}");
        return ESP_OK;
    }
    buf[received] = '\0';
    
    ESP_LOGI(TAG, "Received: %s", buf);
    
    /* 解析 JSON */
    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"success\":false,\"message\":\"Invalid JSON\"}");
        return ESP_OK;
    }
    
    cJSON *ssid_item = cJSON_GetObjectItem(root, "ssid");
    cJSON *password_item = cJSON_GetObjectItem(root, "password");
    
    if (!ssid_item || !cJSON_IsString(ssid_item) || strlen(ssid_item->valuestring) == 0) {
        cJSON_Delete(root);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"success\":false,\"message\":\"SSID is required\"}");
        return ESP_OK;
    }
    
    /* 复制 SSID 和密码到局部变量（因为 cJSON_Delete 后指针会失效） */
    char ssid_buf[33] = {0};
    char password_buf[65] = {0};
    strncpy(ssid_buf, ssid_item->valuestring, sizeof(ssid_buf) - 1);
    if (password_item && cJSON_IsString(password_item)) {
        strncpy(password_buf, password_item->valuestring, sizeof(password_buf) - 1);
    }
    
    ESP_LOGI(TAG, "Connecting to: %s", ssid_buf);
    
    /* 重置重试计数 */
    s_wifi_retry_count = 0;
    
    /* 保存配置 */
    save_wifi_config(ssid_buf, password_buf);
    
    /* 先发送响应 */
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"success\":true,\"message\":\"Connecting...\"}");
    
    cJSON_Delete(root);
    
    /* 延迟后开始连接（让响应先发送出去） */
    vTaskDelay(pdMS_TO_TICKS(500));
    
    /* 开始连接 */
    set_state(PORTAL_STATE_CONNECTING);
    
    /* 停止当前 WiFi */
    esp_wifi_stop();
    vTaskDelay(pdMS_TO_TICKS(100));  /* 等待 WiFi 完全停止 */
    
    /* 创建 STA 网络接口（如果还没有） */
    if (!s_sta_netif) {
        s_sta_netif = esp_netif_create_default_wifi_sta();
    }
    
    /* 配置 STA */
    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, ssid_buf, sizeof(wifi_config.sta.ssid) - 1);
    if (strlen(password_buf) > 0) {
        strncpy((char *)wifi_config.sta.password, password_buf, sizeof(wifi_config.sta.password) - 1);
    }
    wifi_config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;  /* 扫描所有信道 */
    wifi_config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;  /* 按信号强度排序 */
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA_PSK;  /* 最低认证模式 */
    
    ESP_LOGI(TAG, "Configuring STA: SSID='%s', Password length=%d", 
             wifi_config.sta.ssid, strlen(password_buf));
    
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    /* esp_wifi_connect() 将在 WIFI_EVENT_STA_START 事件中调用 */
    
    return ESP_OK;
}

/* Captive Portal 重定向处理 */
static esp_err_t http_captive_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Captive portal redirect: %s", req->uri);
    
    /* 重定向到主页 */
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "http://192.168.4.1/");
    httpd_resp_send(req, NULL, 0);
    
    return ESP_OK;
}

/* 状态查询 API */
static esp_err_t http_status_handler(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "state", s_portal_state);
    cJSON_AddBoolToObject(root, "active", s_portal_active);
    cJSON_AddNumberToObject(root, "sta_count", s_connected_sta_count);
    
    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json_str);
    free(json_str);
    
    return ESP_OK;
}

static esp_err_t start_webserver(void)
{
    if (s_http_server) {
        ESP_LOGW(TAG, "Web server already running");
        return ESP_OK;
    }
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 16;
    config.stack_size = 8192;
    config.lru_purge_enable = true;
    
    esp_err_t ret = httpd_start(&s_http_server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(ret));
        return ret;
    }
    
    /* 注册路由 */
    httpd_uri_t uri_get = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = http_get_handler,
    };
    httpd_register_uri_handler(s_http_server, &uri_get);
    
    httpd_uri_t uri_scan = {
        .uri = "/scan",
        .method = HTTP_GET,
        .handler = http_scan_handler,
    };
    httpd_register_uri_handler(s_http_server, &uri_scan);
    
    httpd_uri_t uri_connect = {
        .uri = "/connect",
        .method = HTTP_POST,
        .handler = http_connect_handler,
    };
    httpd_register_uri_handler(s_http_server, &uri_connect);
    
    httpd_uri_t uri_status = {
        .uri = "/status",
        .method = HTTP_GET,
        .handler = http_status_handler,
    };
    httpd_register_uri_handler(s_http_server, &uri_status);
    
    /* Captive Portal 支持 - 处理各种平台的检测请求 */
    const char *captive_uris[] = {
        "/generate_204",           /* Android */
        "/gen_204",                /* Android */
        "/hotspot-detect.html",    /* iOS/macOS */
        "/library/test/success.html", /* iOS */
        "/ncsi.txt",               /* Windows */
        "/connecttest.txt",        /* Windows */
        "/redirect",               /* Windows */
        "/favicon.ico",
    };
    
    for (int i = 0; i < sizeof(captive_uris) / sizeof(captive_uris[0]); i++) {
        httpd_uri_t uri = {
            .uri = captive_uris[i],
            .method = HTTP_GET,
            .handler = http_captive_handler,
        };
        httpd_register_uri_handler(s_http_server, &uri);
    }
    
    ESP_LOGI(TAG, "Web server started on port %d", config.server_port);
    set_state(PORTAL_STATE_WEB_READY);
    
    return ESP_OK;
}

static void stop_webserver(void)
{
    if (s_http_server) {
        httpd_stop(s_http_server);
        s_http_server = NULL;
        ESP_LOGI(TAG, "Web server stopped");
    }
}

/******************** 公共函数实现 ********************/

esp_err_t captive_portal_init(const captive_portal_config_t *config)
{
    esp_err_t ret;
    
    /* 初始化 NVS */
    ret = init_nvs();
    if (ret != ESP_OK) {
        return ret;
    }
    
    /* 创建事件组 */
    if (!s_wifi_event_group) {
        s_wifi_event_group = xEventGroupCreate();
    }
    
    /* 保存配置 */
    if (config) {
        if (config->ap_ssid && strlen(config->ap_ssid) > 0) {
            strncpy(s_ap_ssid, config->ap_ssid, sizeof(s_ap_ssid) - 1);
        } else {
            strncpy(s_ap_ssid, DEFAULT_AP_SSID, sizeof(s_ap_ssid) - 1);
        }
        
        if (config->ap_password && strlen(config->ap_password) >= 8) {
            strncpy(s_ap_password, config->ap_password, sizeof(s_ap_password) - 1);
        } else {
            strncpy(s_ap_password, DEFAULT_AP_PASSWORD, sizeof(s_ap_password) - 1);
        }
        
        s_ap_channel = config->ap_channel > 0 ? config->ap_channel : DEFAULT_AP_CHANNEL;
        s_state_cb = config->state_cb;
        s_user_data = config->user_data;
    } else {
        strncpy(s_ap_ssid, DEFAULT_AP_SSID, sizeof(s_ap_ssid) - 1);
        strncpy(s_ap_password, DEFAULT_AP_PASSWORD, sizeof(s_ap_password) - 1);
        s_ap_channel = DEFAULT_AP_CHANNEL;
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
    
    ESP_LOGI(TAG, "Captive Portal initialized, AP SSID: %s", s_ap_ssid);
    return ESP_OK;
}

esp_err_t captive_portal_start(void)
{
    esp_err_t ret;
    
    if (s_portal_active) {
        ESP_LOGW(TAG, "Portal already active");
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
            .channel = s_ap_channel,
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
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        wifi_config.ap.password[0] = '\0';
    } else {
        strncpy((char *)wifi_config.ap.password, s_ap_password, sizeof(wifi_config.ap.password));
    }
    
    /* 设置 AP 模式并启动 */
    ret = esp_wifi_set_mode(WIFI_MODE_APSTA);  /* 使用 APSTA 模式以便扫描 */
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set APSTA mode: %s", esp_err_to_name(ret));
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
    
    /* 启动 Web 服务器 */
    ret = start_webserver();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start web server");
        esp_wifi_stop();
        return ret;
    }
    
    s_portal_active = true;
    s_connected_sta_count = 0;
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Captive Portal Started!");
    ESP_LOGI(TAG, "  SSID: %s", s_ap_ssid);
    ESP_LOGI(TAG, "  Pass: %s", strlen(s_ap_password) >= 8 ? s_ap_password : "(Open)");
    ESP_LOGI(TAG, "  URL:  http://%s", s_ap_ip);
    ESP_LOGI(TAG, "========================================");
    
    return ESP_OK;
}

esp_err_t captive_portal_stop(void)
{
    if (!s_portal_active) {
        return ESP_OK;
    }
    
    /* 停止 Web 服务器 */
    stop_webserver();
    
    /* 停止 WiFi */
    esp_wifi_stop();
    
    s_portal_active = false;
    s_connected_sta_count = 0;
    set_state(PORTAL_STATE_IDLE);
    
    ESP_LOGI(TAG, "Captive Portal stopped");
    return ESP_OK;
}

captive_portal_state_t captive_portal_get_state(void)
{
    return s_portal_state;
}

bool captive_portal_is_active(void)
{
    return s_portal_active;
}

const char* captive_portal_get_ap_ssid(void)
{
    return s_ap_ssid;
}

const char* captive_portal_get_ap_ip(void)
{
    return s_ap_ip;
}

uint8_t captive_portal_get_sta_count(void)
{
    return s_connected_sta_count;
}

uint16_t captive_portal_scan_wifi(wifi_scan_result_t *results, uint16_t max_results)
{
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false,
    };
    
    esp_err_t ret = esp_wifi_scan_start(&scan_config, true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi scan failed: %s", esp_err_to_name(ret));
        return 0;
    }
    
    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);
    
    if (ap_count == 0 || !results) {
        return 0;
    }
    
    if (ap_count > max_results) {
        ap_count = max_results;
    }
    
    wifi_ap_record_t *ap_records = malloc(sizeof(wifi_ap_record_t) * ap_count);
    if (!ap_records) {
        return 0;
    }
    
    esp_wifi_scan_get_ap_records(&ap_count, ap_records);
    
    uint16_t valid_count = 0;
    for (int i = 0; i < ap_count && valid_count < max_results; i++) {
        if (strlen((char *)ap_records[i].ssid) > 0) {
            strncpy(results[valid_count].ssid, (char *)ap_records[i].ssid, 32);
            results[valid_count].ssid[32] = '\0';
            results[valid_count].rssi = ap_records[i].rssi;
            results[valid_count].authmode = ap_records[i].authmode;
            valid_count++;
        }
    }
    
    free(ap_records);
    return valid_count;
}

esp_err_t captive_portal_connect_wifi(const char *ssid, const char *password)
{
    if (!ssid || strlen(ssid) == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    /* 重置重试计数 */
    s_wifi_retry_count = 0;
    
    /* 保存配置 */
    save_wifi_config(ssid, password ? password : "");
    
    /* 停止 Portal */
    captive_portal_stop();
    
    /* 创建 STA 网络接口 */
    if (!s_sta_netif) {
        s_sta_netif = esp_netif_create_default_wifi_sta();
    }
    
    /* 配置 STA */
    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    if (password) {
        strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    }
    
    /* 先设置状态，这样 STA_START 事件处理器知道需要连接 */
    set_state(PORTAL_STATE_CONNECTING);
    
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    /* esp_wifi_connect() 将在 WIFI_EVENT_STA_START 事件中调用 */
    
    ESP_LOGI(TAG, "Connecting to WiFi: %s", ssid);
    return ESP_OK;
}

esp_err_t captive_portal_get_saved_ssid(char *ssid, size_t max_len)
{
    char password[65];
    return load_wifi_config(ssid, max_len, password, sizeof(password));
}

bool captive_portal_has_saved_config(void)
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

esp_err_t captive_portal_clear_config(void)
{
    nvs_handle_t handle;
    esp_err_t ret;
    
    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        return ret;
    }
    
    nvs_erase_key(handle, NVS_KEY_SSID);
    nvs_erase_key(handle, NVS_KEY_PASSWORD);
    nvs_commit(handle);
    nvs_close(handle);
    
    ESP_LOGI(TAG, "WiFi config cleared");
    return ESP_OK;
}

esp_err_t captive_portal_auto_connect(void)
{
    char ssid[33] = {0};
    char password[65] = {0};
    
    esp_err_t ret = load_wifi_config(ssid, sizeof(ssid), password, sizeof(password));
    if (ret != ESP_OK || strlen(ssid) == 0) {
        return ESP_ERR_NOT_FOUND;
    }
    
    ESP_LOGI(TAG, "Auto-connecting to saved WiFi: %s", ssid);
    return captive_portal_connect_wifi(ssid, password);
}

