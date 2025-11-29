/**
 * @file main.c
 * @brief Multi-mode application with button switching
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "esp_lcd_axs15231b.h"
#include "user_config.h"
#include "i2c_bsp.h"
#include "lcd_bl_pwm_bsp.h"
#include "button_bsp.h"
#include "app_mode.h"
#include "ui_screens.h"
#include "captive_portal_bsp.h"
#include "esp_netif.h"
#include "esp_wifi.h"

static const char *TAG = "app_main";
static SemaphoreHandle_t lvgl_mux = NULL;   
static SemaphoreHandle_t flush_done_semaphore = NULL; 
static uint16_t *trans_buf_1;

/* 配网模式标志 */
static volatile bool s_wifi_provision_mode = false;

/* WiFi 连接状态 */
static volatile bool s_wifi_connected = false;
static volatile bool s_wifi_auto_connecting = false;

#define LCD_BIT_PER_PIXEL 16
#define BYTES_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565))
#define BUFF_SIZE (EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * BYTES_PER_PIXEL)
#define LVGL_TICK_PERIOD_MS    5
#define LVGL_TASK_MAX_DELAY_MS 500
#define LVGL_TASK_MIN_DELAY_MS 10
#define LVGL_TASK_STACK_SIZE   (8 * 1024)
#define LVGL_TASK_PRIORITY     2

static const axs15231b_lcd_init_cmd_t lcd_init_cmds[] = {
    {0x11, (uint8_t []){0x00}, 0, 100},
    {0x29, (uint8_t []){0x00}, 0, 100},
};

/******************** LVGL 回调函数 ********************/
static bool example_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
    BaseType_t high_task_awoken = pdFALSE;
    xSemaphoreGiveFromISR(flush_done_semaphore, &high_task_awoken);
    return false;
}

static void example_lvgl_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * color_p) {
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)lv_display_get_user_data(disp);
    lv_draw_sw_rgb565_swap(color_p, lv_area_get_width(area) * lv_area_get_height(area));
    const int flush_coun = (LVGL_SPIRAM_BUFF_LEN / LVGL_DMA_BUFF_LEN);
    const int offgap = (EXAMPLE_LCD_V_RES / flush_coun);
    const int dmalen = (LVGL_DMA_BUFF_LEN / 2);
    int offsetx1 = 0, offsety1 = 0, offsetx2 = EXAMPLE_LCD_H_RES, offsety2 = offgap;
    uint16_t *map = (uint16_t *)color_p;
    xSemaphoreGive(flush_done_semaphore);
    for(int i = 0; i<flush_coun; i++) {
        xSemaphoreTake(flush_done_semaphore, portMAX_DELAY);
        memcpy(trans_buf_1, map, LVGL_DMA_BUFF_LEN);
        esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2, offsety2, trans_buf_1);
        offsety1 += offgap; offsety2 += offgap; map += dmalen;
    }
    xSemaphoreTake(flush_done_semaphore, portMAX_DELAY);
    lv_disp_flush_ready(disp);
}

static void example_increase_lvgl_tick(void *arg) { lv_tick_inc(LVGL_TICK_PERIOD_MS); }

/******************** LVGL 锁管理 ********************/
bool example_lvgl_lock(int timeout_ms) {
    assert(lvgl_mux);
    const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTake(lvgl_mux, timeout_ticks) == pdTRUE;
}

void example_lvgl_unlock(void) { assert(lvgl_mux); xSemaphoreGive(lvgl_mux); }

/******************** LVGL 任务 ********************/
static void example_lvgl_port_task(void *arg) {
    uint32_t task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
    for(;;) {
        if (example_lvgl_lock(-1)) { task_delay_ms = lv_timer_handler(); example_lvgl_unlock(); }
        if (task_delay_ms > LVGL_TASK_MAX_DELAY_MS) task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
        else if (task_delay_ms < LVGL_TASK_MIN_DELAY_MS) task_delay_ms = LVGL_TASK_MIN_DELAY_MS;
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
}

/******************** WiFi 配网状态回调 ********************/
static void captive_portal_state_callback(captive_portal_state_t state, void *user_data)
{
    const char *state_str = "Unknown";
    switch (state) {
        case PORTAL_STATE_IDLE:
            state_str = "Idle";
            break;
        case PORTAL_STATE_AP_STARTED:
            state_str = "AP Started";
            if (example_lvgl_lock(100)) {
                ui_wifi_update_provision_info(captive_portal_get_ap_ssid(), 
                                              captive_portal_get_ap_ip());
                example_lvgl_unlock();
            }
            break;
        case PORTAL_STATE_WEB_READY:
            state_str = "Web Server Ready";
            if (example_lvgl_lock(100)) {
                ui_wifi_update_status(false, "Web server ready");
                example_lvgl_unlock();
            }
            break;
        case PORTAL_STATE_STA_CONNECTED:
            state_str = "Device Connected";
            if (example_lvgl_lock(100)) {
                ui_wifi_update_status(false, "Device connected to AP");
                example_lvgl_unlock();
            }
            break;
        case PORTAL_STATE_CONFIGURING:
            state_str = "Configuring...";
            break;
        case PORTAL_STATE_CONNECTING:
            state_str = "Connecting to WiFi...";
            if (example_lvgl_lock(100)) {
                ui_wifi_update_status(false, "Connecting...");
                example_lvgl_unlock();
            }
            break;
        case PORTAL_STATE_CONNECTED:
            state_str = "Connected!";
            s_wifi_provision_mode = false;
            s_wifi_connected = true;
            s_wifi_auto_connecting = false;
            if (example_lvgl_lock(100)) {
                ui_wifi_update_status(true, "WiFi Connected!");
                example_lvgl_unlock();
            }
            /* WiFi 连接成功后，通过串口输出设备信息（JSON 格式） */
            {
                /* 获取设备信息 */
                esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
                esp_netif_ip_info_t ip_info;
                char ip_str[16] = "0.0.0.0";
                if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
                    snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip_info.ip));
                }
                
                uint8_t mac[6];
                char mac_str[18] = "00:00:00:00:00:00";
                if (esp_wifi_get_mac(WIFI_IF_STA, mac) == ESP_OK) {
                    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                }
                
                /* 通过串口输出 JSON 格式的设备信息 */
                printf("\n");
                printf("=== DEVICE_INFO_START ===\n");
                printf("{\"type\":\"device_report\",\"name\":\"Skydimo\",\"device_type\":\"SK-LCD\",\"ip\":\"%s\",\"mac\":\"%s\",\"status\":\"connected\"}\n", ip_str, mac_str);
                printf("=== DEVICE_INFO_END ===\n");
                printf("\n");
                
                ESP_LOGI(TAG, "Device info reported via serial port");
            }
            /* 连接成功后自动切换到时钟模式 */
            vTaskDelay(pdMS_TO_TICKS(2000));
            app_mode_set(APP_MODE_CLOCK);
            if (example_lvgl_lock(100)) {
                ui_screens_switch_to(APP_MODE_CLOCK);
                example_lvgl_unlock();
            }
            app_mode_clear_changed();
            break;
        case PORTAL_STATE_FAILED:
            state_str = "Connection Failed";
            s_wifi_connected = false;
            s_wifi_auto_connecting = false;
            if (example_lvgl_lock(100)) {
                ui_wifi_update_status(false, "Failed! Re-provisioning...");
                example_lvgl_unlock();
            }
            /* 连接失败后自动进入配网模式 */
            ESP_LOGW(TAG, "Auto-connect failed, will start provision mode in 3 seconds...");
            break;
    }
    ESP_LOGI(TAG, "Captive Portal state: %s", state_str);
}

/******************** 进入配网模式 ********************/
static void enter_wifi_provision_mode(void)
{
    if (s_wifi_provision_mode) {
        ESP_LOGW(TAG, "Already in provision mode");
        return;
    }
    
    ESP_LOGI(TAG, ">>> Entering WiFi Provision Mode (SoftAP + Web) <<<");
    s_wifi_provision_mode = true;
    
    /* 初始化 Captive Portal */
    captive_portal_config_t portal_config = {
        .ap_ssid = "Skydimo_Config",
        .ap_password = "12345678",
        .ap_channel = 1,
        .state_cb = captive_portal_state_callback,
        .user_data = NULL,
    };
    
    esp_err_t ret = captive_portal_init(&portal_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init Captive Portal: %s", esp_err_to_name(ret));
        s_wifi_provision_mode = false;
        return;
    }
    
    /* 切换到配网界面 */
    app_mode_set(APP_MODE_WIFI_CONFIG);
    if (example_lvgl_lock(100)) {
        ui_screens_switch_to(APP_MODE_WIFI_CONFIG);
        ui_wifi_set_provision_mode(true);
        example_lvgl_unlock();
    }
    app_mode_clear_changed();
    
    /* 启动 SoftAP + Web 服务器 */
    ret = captive_portal_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start Captive Portal: %s", esp_err_to_name(ret));
        s_wifi_provision_mode = false;
    }
}

/******************** 退出配网模式 ********************/
static void exit_wifi_provision_mode(void)
{
    if (!s_wifi_provision_mode) {
        return;
    }
    
    ESP_LOGI(TAG, "<<< Exiting WiFi Provision Mode >>>");
    
    captive_portal_stop();
    s_wifi_provision_mode = false;
    
    if (example_lvgl_lock(100)) {
        ui_wifi_set_provision_mode(false);
        example_lvgl_unlock();
    }
    
    /* 切换到时钟模式 */
    app_mode_set(APP_MODE_CLOCK);
    if (example_lvgl_lock(100)) {
        ui_screens_switch_to(APP_MODE_CLOCK);
        example_lvgl_unlock();
    }
    app_mode_clear_changed();
}

/******************** WiFi 自动连接任务 ********************/
static void wifi_auto_connect_task(void *arg)
{
    ESP_LOGI(TAG, "WiFi auto-connect task started");
    
    /* 等待 UI 初始化完成 */
    vTaskDelay(pdMS_TO_TICKS(500));
    
    /* 先初始化 Captive Portal（包含 NVS 初始化） */
    captive_portal_config_t portal_config = {
        .ap_ssid = "Skydimo_Config",
        .ap_password = "12345678",
        .ap_channel = 1,
        .state_cb = captive_portal_state_callback,
        .user_data = NULL,
    };
    
    esp_err_t ret = captive_portal_init(&portal_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init Captive Portal: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }
    
    /* 现在检查是否有已保存的 WiFi 配置（NVS 已初始化） */
    if (!captive_portal_has_saved_config()) {
        ESP_LOGI(TAG, "No saved WiFi config found");
        if (example_lvgl_lock(100)) {
            ui_wifi_update_status(false, "No saved WiFi");
            example_lvgl_unlock();
        }
        /* 没有保存的配置，直接进入配网模式 */
        ESP_LOGI(TAG, "Starting provision mode...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        enter_wifi_provision_mode();
        vTaskDelete(NULL);
        return;
    }
    
    /* 获取已保存的 SSID */
    char saved_ssid[33] = {0};
    if (captive_portal_get_saved_ssid(saved_ssid, sizeof(saved_ssid)) == ESP_OK) {
        ESP_LOGI(TAG, "Found saved WiFi config: %s", saved_ssid);
    }
    
    /* 更新 UI 显示正在连接 */
    s_wifi_auto_connecting = true;
    if (example_lvgl_lock(100)) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Connecting: %s", saved_ssid);
        ui_wifi_update_status(false, msg);
        example_lvgl_unlock();
    }
    
    /* 尝试自动连接已保存的 WiFi */
    ESP_LOGI(TAG, "Auto-connecting to saved WiFi: %s", saved_ssid);
    ret = captive_portal_auto_connect();
    
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Auto-connect start failed: %s", esp_err_to_name(ret));
        s_wifi_auto_connecting = false;
    }
    
    /* 等待连接结果（最多等待 15 秒） */
    int timeout_count = 0;
    const int max_timeout = 15;  /* 15 秒超时 */
    
    while (timeout_count < max_timeout) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        timeout_count++;
        
        captive_portal_state_t state = captive_portal_get_state();
        
        if (state == PORTAL_STATE_CONNECTED) {
            ESP_LOGI(TAG, "WiFi connected successfully!");
            vTaskDelete(NULL);
            return;
        }
        
        if (state == PORTAL_STATE_FAILED) {
            ESP_LOGW(TAG, "WiFi connection failed");
            break;
        }
    }
    
    /* 连接失败或超时，自动进入配网模式 */
    if (!s_wifi_connected) {
        ESP_LOGW(TAG, "Auto-connect failed, starting provision mode in 3 seconds...");
        s_wifi_auto_connecting = false;
        
        if (example_lvgl_lock(100)) {
            ui_wifi_update_status(false, "Failed! Starting provision...");
            example_lvgl_unlock();
        }
        
        vTaskDelay(pdMS_TO_TICKS(3000));
        enter_wifi_provision_mode();
    }
    
    vTaskDelete(NULL);
}

/******************** 按键处理任务 ********************/
static void button_handler_task(void *arg) {
    ESP_LOGI(TAG, "Button handler started. BOOT=next mode, PWR=prev mode");
    ESP_LOGI(TAG, "Long press BOOT to enter WiFi provision mode");
    
    for (;;) {
        /* 检测 BOOT 按键事件 */
        EventBits_t boot_bits = xEventGroupWaitBits(boot_key_event, 
            KEY_EVENT_SINGLE_CLICK | KEY_EVENT_DOUBLE_CLICK | KEY_EVENT_LONG_PRESS, 
            pdTRUE, pdFALSE, 0);
        
        /* BOOT 按键长按 - 进入/退出配网模式 */
        if (boot_bits & KEY_EVENT_LONG_PRESS) {
            if (!s_wifi_provision_mode) {
                enter_wifi_provision_mode();
            } else {
                exit_wifi_provision_mode();
            }
        }
        /* BOOT 按键单击 - 切换到下一个模式（非配网模式时） */
        else if ((boot_bits & KEY_EVENT_SINGLE_CLICK) && !s_wifi_provision_mode) {
            app_mode_next();
            ESP_LOGI(TAG, "BOOT short press: switch to next mode");
        }
        
        /* 检测 PWR 按键事件 */
        EventBits_t pwr_bits = xEventGroupWaitBits(pwr_key_event, 
            KEY_EVENT_SINGLE_CLICK | KEY_EVENT_DOUBLE_CLICK | KEY_EVENT_LONG_PRESS, 
            pdTRUE, pdFALSE, 0);
        
        /* PWR 按键长按 - 也可以退出配网模式 */
        if ((pwr_bits & KEY_EVENT_LONG_PRESS) && s_wifi_provision_mode) {
            exit_wifi_provision_mode();
        }
        /* PWR 按键单击 - 切换到上一个模式（非配网模式时） */
        else if ((pwr_bits & KEY_EVENT_SINGLE_CLICK) && !s_wifi_provision_mode) {
            app_mode_prev();
            ESP_LOGI(TAG, "PWR short press: switch to prev mode");
        }
        
        /* 检查模式是否变化，更新 UI */
        if (app_mode_is_changed()) {
            if (example_lvgl_lock(100)) {
                ui_screens_switch_to(app_mode_get_current());
                example_lvgl_unlock();
            }
            app_mode_clear_changed();
        }
        
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

/******************** 时钟更新任务（模拟） ********************/
static void clock_update_task(void *arg) {
    uint8_t hour = 12, minute = 0, second = 0;
    
    for (;;) {
        /* 更新时间 */
        second++;
        if (second >= 60) {
            second = 0;
            minute++;
            if (minute >= 60) {
                minute = 0;
                hour++;
                if (hour >= 24) {
                    hour = 0;
                }
            }
        }
        
        /* 仅在时钟模式时更新显示 */
        if (app_mode_get_current() == APP_MODE_CLOCK) {
            if (example_lvgl_lock(100)) {
                ui_clock_update_time(hour, minute, second);
                example_lvgl_unlock();
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/******************** 主函数 ********************/
void app_main(void) {
    /* 初始化背光 */
    lcd_bl_pwm_bsp_init(LCD_PWM_MODE_255);
    
    /* 创建信号量 */
    flush_done_semaphore = xSemaphoreCreateBinary();
    assert(flush_done_semaphore);
    
    /* 初始化 I2C 和按键 */
    touch_i2c_master_Init();
    button_bsp_init();
    
    /* 初始化 SPI 总线 */
    ESP_LOGI(TAG, "Initialize SPI bus");
    gpio_config_t gpio_conf = {
        .intr_type = GPIO_INTR_DISABLE, 
        .mode = GPIO_MODE_OUTPUT, 
        .pin_bit_mask = ((uint64_t)0x01 << EXAMPLE_PIN_NUM_LCD_RST), 
        .pull_down_en = GPIO_PULLDOWN_DISABLE, 
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));
    
    spi_bus_config_t buscfg = {
        .sclk_io_num = EXAMPLE_PIN_NUM_LCD_PCLK, 
        .data0_io_num = EXAMPLE_PIN_NUM_LCD_DATA0, 
        .data1_io_num = EXAMPLE_PIN_NUM_LCD_DATA1, 
        .data2_io_num = EXAMPLE_PIN_NUM_LCD_DATA2, 
        .data3_io_num = EXAMPLE_PIN_NUM_LCD_DATA3, 
        .max_transfer_sz = LVGL_DMA_BUFF_LEN
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));
    
    /* 初始化 LCD Panel IO */
    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t panel_io = NULL;
    esp_lcd_panel_handle_t panel = NULL;
    
    esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = EXAMPLE_PIN_NUM_LCD_CS, 
        .dc_gpio_num = -1, 
        .spi_mode = 3, 
        .pclk_hz = 40000000, 
        .trans_queue_depth = 10, 
        .on_color_trans_done = example_notify_lvgl_flush_ready, 
        .lcd_cmd_bits = 32, 
        .lcd_param_bits = 8, 
        .flags.quad_mode = true
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(LCD_HOST, &io_config, &panel_io));
    
    /* 初始化 LCD Panel */
    axs15231b_vendor_config_t vendor_config = {
        .flags.use_qspi_interface = 1, 
        .init_cmds = lcd_init_cmds, 
        .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0])
    };
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = -1, 
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB, 
        .bits_per_pixel = LCD_BIT_PER_PIXEL, 
        .vendor_config = &vendor_config
    };
    
    ESP_LOGI(TAG, "Install panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_axs15231b(panel_io, &panel_config, &panel));
    
    /* LCD 复位 */
    ESP_ERROR_CHECK(gpio_set_level(EXAMPLE_PIN_NUM_LCD_RST, 1)); 
    vTaskDelay(pdMS_TO_TICKS(30));
    ESP_ERROR_CHECK(gpio_set_level(EXAMPLE_PIN_NUM_LCD_RST, 0)); 
    vTaskDelay(pdMS_TO_TICKS(250));
    ESP_ERROR_CHECK(gpio_set_level(EXAMPLE_PIN_NUM_LCD_RST, 1)); 
    vTaskDelay(pdMS_TO_TICKS(30));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));

    /* 初始化 LVGL */
    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();
    
    lv_display_t *disp = lv_display_create(EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    lv_display_set_flush_cb(disp, example_lvgl_flush_cb);
    
    uint8_t *buffer_1 = (uint8_t *)heap_caps_malloc(BUFF_SIZE, MALLOC_CAP_SPIRAM); 
    assert(buffer_1);
    uint8_t *buffer_2 = (uint8_t *)heap_caps_malloc(BUFF_SIZE, MALLOC_CAP_SPIRAM); 
    assert(buffer_2);
    trans_buf_1 = (uint16_t *)heap_caps_malloc(LVGL_DMA_BUFF_LEN, MALLOC_CAP_DMA); 
    assert(trans_buf_1);
    
    lv_display_set_buffers(disp, buffer_1, buffer_2, BUFF_SIZE, LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_user_data(disp, panel);

    /* 创建 LVGL tick 定时器 */
    esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick, 
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));

    /* 创建 LVGL 互斥锁和任务 */
    lvgl_mux = xSemaphoreCreateMutex(); 
    assert(lvgl_mux);
    xTaskCreatePinnedToCore(example_lvgl_port_task, "LVGL", LVGL_TASK_STACK_SIZE, NULL, LVGL_TASK_PRIORITY, NULL, 0);
    
    /* 初始化 UI 界面 */
    if (example_lvgl_lock(-1)) { 
        ui_screens_init();
        ui_screens_switch_to(APP_MODE_CLOCK);
        example_lvgl_unlock(); 
    }
    
    /* 创建按键处理任务 */
    xTaskCreate(button_handler_task, "button_handler", 4096, NULL, 3, NULL);
    
    /* 创建时钟更新任务 */
    xTaskCreate(clock_update_task, "clock_update", 2048, NULL, 2, NULL);
    
    /* 创建 WiFi 自动连接任务（如果有保存的配置则自动连接） */
    xTaskCreate(wifi_auto_connect_task, "wifi_auto", 4096, NULL, 2, NULL);
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Skydimo Multi-mode Application");
    ESP_LOGI(TAG, "----------------------------------------");
    ESP_LOGI(TAG, "  BOOT short: Next mode");
    ESP_LOGI(TAG, "  PWR short:  Previous mode");
    ESP_LOGI(TAG, "  BOOT long:  Enter/Exit WiFi Provision");
    ESP_LOGI(TAG, "----------------------------------------");
    ESP_LOGI(TAG, "  WiFi: SoftAP + Web Configuration");
    ESP_LOGI(TAG, "  Connect to 'Skydimo_Config'");
    ESP_LOGI(TAG, "  Saved WiFi config auto-connects!");
    ESP_LOGI(TAG, "========================================");
}
