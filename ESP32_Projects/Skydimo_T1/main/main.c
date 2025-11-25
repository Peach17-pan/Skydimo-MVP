#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/spi_master.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "driver/gpio.h"

#include "lvgl.h"
#include "user_config.h"
#include "i2c_bsp.h"
#include "lcd_bl_pwm_bsp.h"
#include "mode_manager.h"
#include "key_detector.h"
#include "ui_manager.h"

static const char *TAG = "main";
static SemaphoreHandle_t lvgl_mux = NULL;
static SemaphoreHandle_t lvgl_flush_semap;

#if (Rotated == USER_DISP_ROT_90)
uint16_t* rotat_ptr = NULL;
#endif

// ========== 函数声明 ==========
static bool example_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx);
static void example_increase_lvgl_tick(void *arg);
static void send_lcd_init_commands(esp_lcd_panel_io_handle_t io_handle);
static void test_simple_color_fill(esp_lcd_panel_io_handle_t io_handle);
static void example_lvgl_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * color_p);
static bool example_lvgl_lock(int timeout_ms);
static void example_lvgl_unlock(void);
void example_lvgl_port_task(void *arg);
static void example_lvgl_touch_cb(lv_indev_t * indev, lv_indev_data_t *data);
static void example_backlight_loop_task(void *arg);

// 按键回调函数
static void short_press_callback(void) {
    ESP_LOGI(TAG, "Short press - Switch mode");
    switch_to_next_mode();
    device_mode_t current_mode = get_current_mode();
    ui_update_current_mode(current_mode);
}

static void long_press_callback(void) {
    ESP_LOGI(TAG, "Long press - Enter network config");
    switch_to_mode(MODE_NETWORK_CONFIG);
    ui_update_current_mode(MODE_NETWORK_CONFIG);
}

// AXS15231B初始化命令序列
typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t datalen;
    uint8_t delay_ms;
} lcd_init_cmd_t;

static const lcd_init_cmd_t lcd_init_cmds[] = {
    {0x01, {0x00}, 0, 120},                    // 软件复位
    {0x11, {0x00}, 0, 120},                    // 退出睡眠模式
    {0x3A, {0x55}, 1, 10},                     // 像素格式设置 - RGB565
    {0x36, {0x00}, 1, 10},                     // 内存访问控制
    {0x2A, {0x00, 0x00, 0x00, 0xAB}, 4, 10},   // 设置列地址 (0-171)
    {0x2B, {0x00, 0x00, 0x01, 0xDF}, 4, 10},   // 设置行地址 (0-479)
    {0xB1, {0x00, 0x1F, 0x0F}, 3, 10},         // 帧率控制
    {0xC0, {0x1B, 0x1B}, 2, 10},               // 电源控制1
    {0xC1, {0x45}, 1, 10},                     // 电源控制2
    {0xC5, {0x38, 0x80}, 2, 10},               // VCOM控制
    {0x29, {0x00}, 0, 100},                    // 开启显示
};

// ========== 函数实现 ==========

static bool example_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    BaseType_t TaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(lvgl_flush_semap, &TaskWoken);
    return (TaskWoken == pdTRUE);
}

static void example_increase_lvgl_tick(void *arg)
{
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

// 发送LCD初始化命令
static void send_lcd_init_commands(esp_lcd_panel_io_handle_t io_handle)
{
    ESP_LOGI(TAG, "Sending LCD initialization commands");
    
    for (int i = 0; i < sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]); i++) {
        const lcd_init_cmd_t *cmd = &lcd_init_cmds[i];
        
        if (cmd->datalen > 0) {
            esp_lcd_panel_io_tx_param(io_handle, cmd->cmd, cmd->data, cmd->datalen);
        } else {
            uint8_t dummy = 0;
            esp_lcd_panel_io_tx_param(io_handle, cmd->cmd, &dummy, 0);
        }
        
        if (cmd->delay_ms > 0) {
            vTaskDelay(pdMS_TO_TICKS(cmd->delay_ms));
        }
    }
    
    ESP_LOGI(TAG, "LCD initialization commands sent");
}

// 简单的颜色填充测试
static void test_simple_color_fill(esp_lcd_panel_io_handle_t io_handle)
{
    ESP_LOGI(TAG, "Starting simple color fill test");
    
    // 设置列地址 (0-171)
    uint8_t caset[] = {0x00, 0x00, 0x00, 0xAB};
    // 设置行地址 (0-479)
    uint8_t raset[] = {0x00, 0x00, 0x01, 0xDF};
    
    esp_lcd_panel_io_tx_param(io_handle, 0x2A, caset, 4);
    esp_lcd_panel_io_tx_param(io_handle, 0x2B, raset, 4);
    
    // 发送内存写命令
    esp_lcd_panel_io_tx_param(io_handle, 0x2C, NULL, 0);
    
    // 填充红色
    uint16_t red_color = 0xF800; // RGB565红色
    uint16_t line_buffer[172];
    for (int i = 0; i < 172; i++) {
        line_buffer[i] = red_color;
    }
    
    for (int y = 0; y < 480; y++) {
        esp_lcd_panel_io_tx_color(io_handle, 0x2C, line_buffer, 172 * sizeof(uint16_t));
    }
    
    ESP_LOGI(TAG, "Color fill test completed");
}

// LVGL v8.3+ 兼容的刷新回调
static void example_lvgl_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * color_p)
{
    esp_lcd_panel_io_handle_t panel_io = (esp_lcd_panel_io_handle_t)lv_display_get_user_data(disp);
    
    if (panel_io == NULL) {
        lv_display_flush_ready(disp);
        return;
    }

    int32_t x1 = area->x1;
    int32_t y1 = area->y1;
    int32_t x2 = area->x2;
    int32_t y2 = area->y2;
    int32_t width = x2 - x1 + 1;
    int32_t height = y2 - y1 + 1;
    size_t len = width * height * 2;  // 16位颜色，2字节每像素

    if (len == 0 || color_p == NULL) {
        lv_display_flush_ready(disp);
        return;
    }

    // 直接发送数据
    esp_lcd_panel_io_tx_color(panel_io, -1, color_p, len);

    // 立即通知刷新完成
    lv_display_flush_ready(disp);
}

static bool example_lvgl_lock(int timeout_ms)
{
    if (lvgl_mux == NULL) return false;
    const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTake(lvgl_mux, timeout_ticks) == pdTRUE;
}

static void example_lvgl_unlock(void)
{
    if (lvgl_mux == NULL) return;
    xSemaphoreGive(lvgl_mux);
}

void example_lvgl_port_task(void *arg)
{
    uint32_t task_delay_ms = EXAMPLE_LVGL_TASK_MAX_DELAY_MS;
    
    for(;;)
    {
        if (example_lvgl_lock(-1)) 
        {
            task_delay_ms = lv_timer_handler();
            example_lvgl_unlock();
        }
        
        if (task_delay_ms > EXAMPLE_LVGL_TASK_MAX_DELAY_MS)
        {
            task_delay_ms = EXAMPLE_LVGL_TASK_MAX_DELAY_MS;
        } 
        else if (task_delay_ms < EXAMPLE_LVGL_TASK_MIN_DELAY_MS)
        {
            task_delay_ms = EXAMPLE_LVGL_TASK_MIN_DELAY_MS;
        }
        
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
}

// LVGL v8.3+ 兼容的触摸回调
static void example_lvgl_touch_cb(lv_indev_t * indev, lv_indev_data_t *data)
{
    if (disp_touch_dev_handle == NULL) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    uint8_t read_touchpad_cmd[11] = {0xb5, 0xab, 0xa5, 0x5a, 0x0, 0x0, 0x0, 0x0e, 0x0, 0x0, 0x0};
    uint8_t buff[32] = {0};
    
    // 使用正确的触摸I2C函数
    esp_err_t ret = i2c_master_touch_write_read(disp_touch_dev_handle, read_touchpad_cmd, 11, buff, 32);
    
    if (ret != ESP_OK) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }
    
    uint16_t pointX;
    uint16_t pointY;
    pointX = (((uint16_t)buff[2] & 0x0f) << 8) | (uint16_t)buff[3];
    pointY = (((uint16_t)buff[4] & 0x0f) << 8) | (uint16_t)buff[5];
    
    if (buff[1] > 0 && buff[1] < 5)
    {
        data->state = LV_INDEV_STATE_PRESSED;
        
#if (Rotated == USER_DISP_ROT_NONO)
        if(pointX > EXAMPLE_LCD_V_RES) pointX = EXAMPLE_LCD_V_RES;
        if(pointY > EXAMPLE_LCD_H_RES) pointY = EXAMPLE_LCD_H_RES;
        data->point.x = pointY;
        data->point.y = (EXAMPLE_LCD_V_RES - pointX);
#else
        if(pointX > EXAMPLE_LCD_H_RES) pointX = EXAMPLE_LCD_H_RES;
        if(pointY > EXAMPLE_LCD_V_RES) pointY = EXAMPLE_LCD_V_RES;
        data->point.x = (EXAMPLE_LCD_H_RES - pointX);
        data->point.y = (EXAMPLE_LCD_V_RES - pointY);
#endif
    }
    else 
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static void example_backlight_loop_task(void *arg)
{
    for(;;)
    {
#if (Backlight_Testing == 1)
        vTaskDelay(pdMS_TO_TICKS(1500));
        setUpduty(LCD_PWM_MODE_255);
        vTaskDelay(pdMS_TO_TICKS(1500));
        setUpduty(LCD_PWM_MODE_175);
        vTaskDelay(pdMS_TO_TICKS(1500));
        setUpduty(LCD_PWM_MODE_125);
        vTaskDelay(pdMS_TO_TICKS(1500));
        setUpduty(LCD_PWM_MODE_0);
#else
        vTaskDelay(pdMS_TO_TICKS(2000));
#endif
    }
}

// ========== 主函数 ==========
void app_main(void)
{
    ESP_LOGI(TAG, "=== Starting AXS15231B LCD Test ===");
    ESP_LOGI(TAG, "Resolution: %dx%d", EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    ESP_LOGI(TAG, "Rotation: %s", (Rotated == USER_DISP_ROT_90) ? "90 degrees" : "None");
    
    // 初始化模式管理器
    mode_manager_init();
    
    // 初始化背光
    lcd_bl_pwm_bsp_init(LCD_PWM_MODE_255);
    ESP_LOGI(TAG, "Backlight initialized");
    
#if (Rotated == USER_DISP_ROT_90)
    rotat_ptr = (uint16_t*)heap_caps_malloc(EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (rotat_ptr == NULL) {
        ESP_LOGE(TAG, "Failed to allocate rotation buffer");
        return;
    }
    ESP_LOGI(TAG, "Rotation buffer allocated");
#endif

    lvgl_flush_semap = xSemaphoreCreateBinary();
    if (lvgl_flush_semap == NULL) {
        ESP_LOGE(TAG, "Failed to create flush semaphore");
        return;
    }
    
    // 初始化I2C
    touch_i2c_master_Init();
    ESP_LOGI(TAG, "I2C initialized");

    // 配置LCD复位GPIO
    ESP_LOGI(TAG, "Initialize LCD RESET GPIO");
    gpio_config_t gpio_conf = {
        .pin_bit_mask = (1ULL << EXAMPLE_PIN_NUM_LCD_RST),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&gpio_conf));

    ESP_LOGI(TAG, "Initialize QSPI bus");
    spi_bus_config_t buscfg = {
        .data0_io_num = EXAMPLE_PIN_NUM_LCD_DATA0,
        .data1_io_num = EXAMPLE_PIN_NUM_LCD_DATA1,
        .data2_io_num = EXAMPLE_PIN_NUM_LCD_DATA2,
        .data3_io_num = EXAMPLE_PIN_NUM_LCD_DATA3,
        .sclk_io_num = EXAMPLE_PIN_NUM_LCD_PCLK,
        .max_transfer_sz = LVGL_DMA_BUFF_LEN,
        .flags = 0,
        .intr_flags = 0
    };
    
    esp_err_t ret = spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus initialize failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "SPI bus initialized");

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t panel_io = NULL;
    
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = -1,
        .cs_gpio_num = EXAMPLE_PIN_NUM_LCD_CS,
        .pclk_hz = 10 * 1000 * 1000,  // 降低到10MHz确保稳定性
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = example_notify_lvgl_flush_ready,
        .user_ctx = NULL,
    };
    
    // 启用QSPI模式
    io_config.flags.quad_mode = 1;
    
    ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &panel_io);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create panel IO: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "Panel IO installed");

    // 执行硬件复位序列
    ESP_LOGI(TAG, "Performing hardware reset");
    ESP_ERROR_CHECK(gpio_set_level(EXAMPLE_PIN_NUM_LCD_RST, 1));
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_ERROR_CHECK(gpio_set_level(EXAMPLE_PIN_NUM_LCD_RST, 0));
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_ERROR_CHECK(gpio_set_level(EXAMPLE_PIN_NUM_LCD_RST, 1));
    vTaskDelay(pdMS_TO_TICKS(120));
    
    // 发送初始化命令
    send_lcd_init_commands(panel_io);
    ESP_LOGI(TAG, "LCD initialized");

    // 先进行简单的颜色填充测试
    test_simple_color_fill(panel_io);
    vTaskDelay(pdMS_TO_TICKS(2000));

    // 现在初始化LVGL
    ESP_LOGI(TAG, "Initializing LVGL");
    lv_init();

    // 分配显示缓冲区
    static lv_color_t buf1[EXAMPLE_LCD_H_RES * 10];
    static lv_color_t buf2[EXAMPLE_LCD_H_RES * 10];

    // 初始化显示缓冲区 (LVGL v8.3+ API)
    lv_display_t *disp = lv_display_create(EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    if (disp == NULL) {
        ESP_LOGE(TAG, "Failed to create LVGL display");
        return;
    }
    
    lv_display_set_buffers(disp, buf1, buf2, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, example_lvgl_flush_cb);
    lv_display_set_user_data(disp, panel_io);

    ESP_LOGI(TAG, "LVGL display created");

    // 创建LVGL定时器
    ESP_LOGI(TAG, "Install LVGL tick timer");
    esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));

    // 创建输入设备
    ESP_LOGI(TAG, "Register touch input device");
    lv_indev_t *indev = lv_indev_create();
    if (indev) {
        lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(indev, example_lvgl_touch_cb);
        ESP_LOGI(TAG, "Touch input device registered");
    } else {
        ESP_LOGE(TAG, "Failed to create input device");
    }

    // 创建LVGL互斥锁和任务
    lvgl_mux = xSemaphoreCreateMutex();
    if (lvgl_mux == NULL) {
        ESP_LOGE(TAG, "Failed to create LVGL mutex");
        return;
    }
    
    if (xTaskCreate(example_lvgl_port_task, "LVGL", 4096, NULL, 4, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create LVGL task");
        return;
    }
    
    if (xTaskCreate(example_backlight_loop_task, "Backlight", 2048, NULL, 2, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create backlight task");
    }

    // 初始化UI管理器
    ui_manager_init(disp);

    // 初始化按键检测
    key_detector_init(short_press_callback, long_press_callback);
    ESP_LOGI(TAG, "Key detector initialized");

    // 创建初始界面
    if (example_lvgl_lock(-1))
    {
        device_mode_t current_mode = get_current_mode();
        ui_update_current_mode(current_mode);
        example_lvgl_unlock();
        
        ESP_LOGI(TAG, "=== System Ready ===");
        ESP_LOGI(TAG, "Current mode: %s", get_mode_name(current_mode));
    }
    
    // 主循环
    int counter = 0;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        counter++;
        
        // 每10秒打印一次状态
        if (counter % 10 == 0) {
            ESP_LOGI(TAG, "System running... Current mode: %s", get_mode_name(get_current_mode()));
        }
    }
}