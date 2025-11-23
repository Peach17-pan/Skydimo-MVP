#include <stdio.h>
#include <string.h>
#include <inttypes.h>  // 添加这个头文件

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "esp_heap_caps.h"

#include "lvgl.h"
#include "user_config.h"
#include "i2c_bsp.h"
#include "lcd_bl_pwm_bsp.h"
#include "mode_manager.h"
#include "key_detector.h"
#include "ui_manager.h"

static const char *TAG = "main";

static SemaphoreHandle_t lvgl_mux = NULL;   
static SemaphoreHandle_t flush_done_semaphore = NULL; 
uint8_t *lvgl_dest = NULL;
static uint16_t *trans_buf_1;

// LVGL配置
#define LCD_BIT_PER_PIXEL 16
#define BYTES_PER_PIXEL 2

// 减小缓冲区大小以节省内存
#define BUFFER_LINES 40  // 从120减小到40
#define BUFF_SIZE (EXAMPLE_LCD_H_RES * BUFFER_LINES * BYTES_PER_PIXEL)

#define LVGL_TICK_PERIOD_MS    5
#define LVGL_TASK_MAX_DELAY_MS 500
#define LVGL_TASK_MIN_DELAY_MS 20
#define LVGL_TASK_STACK_SIZE   (8 * 1024)  // 改回8KB
#define LVGL_TASK_PRIORITY     2

// 全局变量
static lv_display_t* main_display = NULL;

// LCD初始化命令
typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t data_bytes;
    uint8_t delay_ms;
} lcd_init_cmd_t;

static const lcd_init_cmd_t lcd_init_cmds[] = 
{
    {0x11, {0x00}, 0, 100},
    {0x29, {0x00}, 0, 100},
};

// 函数声明
static bool example_lvgl_lock(int timeout_ms);
static void example_lvgl_unlock(void);
static void example_increase_lvgl_tick(void *arg);
static void example_lvgl_port_task(void *arg);
static void example_backlight_loop_task(void *arg);
static bool example_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx);
static void example_lvgl_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * color_p);
static void TouchInputReadCallback(lv_indev_t * indev, lv_indev_data_t *indevData);
static esp_lcd_panel_io_handle_t initialize_lcd(void);

// LVGL锁函数
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

// LVGL定时器回调
static void example_increase_lvgl_tick(void *arg)
{
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

// 按键回调函数
static void on_key_short_press(void)
{
    ESP_LOGI(TAG, "Short press detected, switching mode");
    switch_to_next_mode();
    
    if (example_lvgl_lock(-1)) {
        ui_update_current_mode(get_current_mode());
        example_lvgl_unlock();
    }
}

static void on_key_long_press(void)
{
    ESP_LOGI(TAG, "Long press detected, entering network config mode");
    switch_to_mode(MODE_NETWORK_CONFIG);
    
    if (example_lvgl_lock(-1)) {
        ui_update_current_mode(get_current_mode());
        example_lvgl_unlock();
    }
}

// 刷新完成回调函数
static bool example_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    BaseType_t high_task_awoken = pdFALSE;
    if (flush_done_semaphore) {
        xSemaphoreGiveFromISR(flush_done_semaphore, &high_task_awoken);
    }
    return high_task_awoken == pdTRUE;
}

// LVGL刷新回调函数 - 简化版本
static void example_lvgl_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * color_p)
{
    esp_lcd_panel_io_handle_t panel_io = (esp_lcd_panel_io_handle_t)lv_display_get_user_data(disp);
    
    // 简化刷新逻辑，避免复杂的内存操作
    int32_t width = lv_area_get_width(area);
    int32_t height = lv_area_get_height(area);
    
    // 直接发送数据，不进行复杂的内存拷贝
    esp_lcd_panel_io_tx_color(panel_io, -1, color_p, width * height * BYTES_PER_PIXEL);
    
    // 立即通知刷新完成
    lv_disp_flush_ready(disp);
}

// 触摸输入回调函数 - 添加错误处理
static void TouchInputReadCallback(lv_indev_t * indev, lv_indev_data_t *indevData)
{
    // 添加触摸设备存在性检查
    if (disp_touch_dev_handle == NULL) {
        indevData->state = LV_INDEV_STATE_RELEASED;
        indevData->point.x = 0;
        indevData->point.y = 0;
        return;
    }
    
    uint8_t read_touchpad_cmd[11] = {0xb5, 0xab, 0xa5, 0x5a, 0x0, 0x0, 0x0, 0x0e,0x0, 0x0, 0x0};
    uint8_t buff[32] = {0};
    
    esp_err_t ret = i2c_master_write_read_dev(disp_touch_dev_handle, read_touchpad_cmd, 11, buff, 32);
    if (ret != ESP_OK) {
        indevData->state = LV_INDEV_STATE_RELEASED;
        return;
    }
    
    uint16_t pointX;
    uint16_t pointY;
    pointX = (((uint16_t)buff[2] & 0x0f) << 8) | (uint16_t)buff[3];
    pointY = (((uint16_t)buff[4] & 0x0f) << 8) | (uint16_t)buff[5];
    
    if (buff[1] > 0 && buff[1] < 5) {
        indevData->state = LV_INDEV_STATE_PRESSED;
#if (Rotated == USER_DISP_ROT_90)
        if (pointX > EXAMPLE_LCD_H_RES) pointX = EXAMPLE_LCD_H_RES;
        if (pointY > EXAMPLE_LCD_V_RES) pointY = EXAMPLE_LCD_V_RES;
        indevData->point.x = (EXAMPLE_LCD_H_RES - pointX);
        indevData->point.y = (EXAMPLE_LCD_V_RES - pointY); 
#else
        if (pointX > EXAMPLE_LCD_V_RES) pointX = EXAMPLE_LCD_V_RES;
        if (pointY > EXAMPLE_LCD_H_RES) pointY = EXAMPLE_LCD_H_RES;
        indevData->point.x = pointY;
        indevData->point.y = (EXAMPLE_LCD_V_RES - pointX);
#endif
    } else {
        indevData->state = LV_INDEV_STATE_RELEASED;
    }
}

// LVGL任务函数 - 添加看门狗喂狗
static void example_lvgl_port_task(void *arg)
{
    // 订阅任务看门狗
    esp_task_wdt_add(NULL);
    
    uint32_t task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
    for(;;) {
        // 喂狗
        esp_task_wdt_reset();
        
        if (example_lvgl_lock(-1)) {
            task_delay_ms = lv_timer_handler();
            example_lvgl_unlock();
        }
        
        if (task_delay_ms > LVGL_TASK_MAX_DELAY_MS) {
            task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
        } else if (task_delay_ms < LVGL_TASK_MIN_DELAY_MS) {
            task_delay_ms = LVGL_TASK_MIN_DELAY_MS;
        }
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
    
    // 取消订阅（理论上不会执行到这里）
    esp_task_wdt_delete(NULL);
}

// 背光控制任务 - 添加看门狗喂狗
static void example_backlight_loop_task(void *arg)
{
    esp_task_wdt_add(NULL);
    
    for(;;) {
        esp_task_wdt_reset();
        
#if (Backlight_Testing == true)
        vTaskDelay(pdMS_TO_TICKS(1500));
        setUpduty(LCD_PWM_MODE_255);
        vTaskDelay(pdMS_TO_TICKS(1500));
        setUpduty(LCD_PWM_MODE_175);
        vTaskDelay(pdMS_TO_TICKS(1500));
        setUpduty(LCD_PWM_MODE_125);
        vTaskDelay(pdMS_TO_TICKS(1500));
        setUpduty(LCD_PWM_MODE_0);
#else
        vTaskDelay(pdMS_TO_TICKS(5000));  // 延长延迟时间
#endif
    }
    
    esp_task_wdt_delete(NULL);
}

// LCD初始化函数
static esp_lcd_panel_io_handle_t initialize_lcd(void)
{
    ESP_LOGI(TAG, "Initialize SPI bus");
    
    // GPIO配置
    gpio_config_t gpio_conf = {};
    gpio_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_conf.mode = GPIO_MODE_OUTPUT;
    gpio_conf.pin_bit_mask = ((uint64_t)0x01<<EXAMPLE_PIN_NUM_LCD_RST);
    gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));

    // SPI总线配置
    spi_bus_config_t buscfg = {};
    buscfg.sclk_io_num =  EXAMPLE_PIN_NUM_LCD_PCLK;  
    buscfg.data0_io_num = EXAMPLE_PIN_NUM_LCD_DATA0;            
    buscfg.data1_io_num = EXAMPLE_PIN_NUM_LCD_DATA1;             
    buscfg.data2_io_num = EXAMPLE_PIN_NUM_LCD_DATA2;
    buscfg.data3_io_num = EXAMPLE_PIN_NUM_LCD_DATA3;
    buscfg.max_transfer_sz = LVGL_DMA_BUFF_LEN;
    
    esp_err_t ret = spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus initialize failed: %s", esp_err_to_name(ret));
        return NULL;
    }
    
    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t panel_io = NULL;
    
    // SPI IO配置
    esp_lcd_panel_io_spi_config_t io_config = {};
    io_config.cs_gpio_num = EXAMPLE_PIN_NUM_LCD_CS;                 
    io_config.dc_gpio_num = -1;          
    io_config.spi_mode = 3;              
    io_config.pclk_hz = 10 * 1000 * 1000;  // 进一步降低到10MHz
    io_config.trans_queue_depth = 5;      // 减小队列深度
    io_config.on_color_trans_done = example_notify_lvgl_flush_ready; 
    io_config.lcd_cmd_bits = 8;           // 改为8位命令
    io_config.lcd_param_bits = 8;         // 改为8位参数
    io_config.flags.quad_mode = true;                         
    
    ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &panel_io);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LCD IO initialize failed: %s", esp_err_to_name(ret));
        return NULL;
    }
    
    // 复位序列
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_set_level(EXAMPLE_PIN_NUM_LCD_RST,1));
    vTaskDelay(pdMS_TO_TICKS(30));
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_set_level(EXAMPLE_PIN_NUM_LCD_RST,0));
    vTaskDelay(pdMS_TO_TICKS(250));
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_set_level(EXAMPLE_PIN_NUM_LCD_RST,1));
    vTaskDelay(pdMS_TO_TICKS(30));
    
    // 发送初始化命令
    for (int i = 0; i < sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]); i++) {
        esp_lcd_panel_io_tx_param(panel_io, lcd_init_cmds[i].cmd, lcd_init_cmds[i].data, lcd_init_cmds[i].data_bytes);
        if (lcd_init_cmds[i].delay_ms > 0) {
            vTaskDelay(pdMS_TO_TICKS(lcd_init_cmds[i].delay_ms));
        }
    }
    
    return panel_io;
}

// 内存分配辅助函数
static void* allocate_memory(size_t size, uint32_t caps, const char* name)
{
    void* ptr = heap_caps_malloc(size, caps);
    if (ptr == NULL) {
        ESP_LOGE(TAG, "Failed to allocate %s (%u bytes)", name, (unsigned int)size);
        
        // 尝试使用默认分配
        ptr = malloc(size);
        if (ptr == NULL) {
            ESP_LOGE(TAG, "Default allocation also failed for %s", name);
        } else {
            ESP_LOGW(TAG, "Used default allocation for %s", name);
        }
    } else {
        ESP_LOGI(TAG, "Successfully allocated %s (%u bytes)", name, (unsigned int)size);
    }
    return ptr;
}

// 主函数 - 修复所有格式化字符串错误
void app_main(void)
{
    // 设置日志级别
    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set("main", ESP_LOG_INFO);
    
    // 检查重启原因
    esp_reset_reason_t reason = esp_reset_reason();
    ESP_LOGI(TAG, "=== System Start ===");
    ESP_LOGI(TAG, "Reset reason: %u", (unsigned int)reason);
    
    // 打印内存信息 - 修复所有格式化字符串
    ESP_LOGI(TAG, "Free heap: %u", (unsigned int)esp_get_free_heap_size());
    ESP_LOGI(TAG, "Largest free block: %u", (unsigned int)heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));
    
    // 移除看门狗初始化（系统已自动初始化）
    ESP_LOGI(TAG, "Task WDT already initialized by system");
    
    ESP_LOGI(TAG, "Starting multi-mode display system...");
    
    // 1. 初始化硬件外设（逐步初始化，便于定位问题）
    ESP_LOGI(TAG, "Step 1: Initialize PWM backlight");
    lcd_bl_pwm_bsp_init(LCD_PWM_MODE_255);
    
    ESP_LOGI(TAG, "Step 2: Create semaphores");
    flush_done_semaphore = xSemaphoreCreateBinary();
    if (flush_done_semaphore == NULL) {
        ESP_LOGE(TAG, "Failed to create flush semaphore");
        return;
    }
    
    ESP_LOGI(TAG, "Step 3: Initialize I2C for touch");
    touch_i2c_master_Init();
    
    // 2. 初始化LCD
    ESP_LOGI(TAG, "Step 4: Initialize LCD");
    esp_lcd_panel_io_handle_t panel_io = initialize_lcd();
    if (panel_io == NULL) {
        ESP_LOGE(TAG, "LCD initialization failed");
        return;
    }
    
    // 3. 初始化LVGL
    ESP_LOGI(TAG, "Step 5: Initialize LVGL");
    lv_init();
    main_display = lv_display_create(EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    if (main_display == NULL) {
        ESP_LOGE(TAG, "Failed to create LVGL display");
        return;
    }
    
    lv_display_set_flush_cb(main_display, example_lvgl_flush_cb);
    
    // 分配显示缓冲区（使用新的内存分配函数）
    ESP_LOGI(TAG, "Step 6: Allocate display buffers");
    
    // 尝试使用SPIRAM，如果失败则使用默认内存
    uint8_t *buffer_1 = allocate_memory(BUFF_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT, "buffer_1");
    if (buffer_1 == NULL) {
        ESP_LOGE(TAG, "Critical: Cannot allocate display buffer 1");
        return;
    }
    
    uint8_t *buffer_2 = allocate_memory(BUFF_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT, "buffer_2");
    if (buffer_2 == NULL) {
        ESP_LOGE(TAG, "Cannot allocate buffer_2, using single buffer mode");
        // 单缓冲模式
        lv_display_set_buffers(main_display, buffer_1, NULL, BUFF_SIZE, LV_DISPLAY_RENDER_MODE_FULL);
    } else {
        // 双缓冲模式
        lv_display_set_buffers(main_display, buffer_1, buffer_2, BUFF_SIZE, LV_DISPLAY_RENDER_MODE_FULL);
    }
    
    lv_display_set_user_data(main_display, panel_io);

    // 分配DMA缓冲区
    trans_buf_1 = allocate_memory(LVGL_DMA_BUFF_LEN, MALLOC_CAP_DMA, "trans_buf_1");
    if (trans_buf_1 == NULL) {
        ESP_LOGW(TAG, "DMA buffer allocation failed, may affect performance");
    }

#if (Rotated == USER_DISP_ROT_90)
    lvgl_dest = allocate_memory(BUFF_SIZE, MALLOC_CAP_SPIRAM, "lvgl_dest");
    if (lvgl_dest) {
        lv_display_set_rotation(main_display, LV_DISPLAY_ROTATION_90);
    }
#endif

    // 4. 初始化模式管理系统
    ESP_LOGI(TAG, "Step 7: Initialize mode manager");
    mode_manager_init();
    ui_manager_init(main_display);
    
    // 5. 初始化按键检测
    ESP_LOGI(TAG, "Step 8: Initialize key detector");
    key_detector_init(on_key_short_press, on_key_long_press);
    
    // 6. 创建触摸输入设备
    ESP_LOGI(TAG, "Step 9: Create touch input");
    lv_indev_t *touch_indev = lv_indev_create();
    if (touch_indev) {
        lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(touch_indev, TouchInputReadCallback);
    }
    
    // 7. 创建LVGL定时器
    ESP_LOGI(TAG, "Step 10: Create LVGL timer");
    esp_timer_create_args_t lvgl_tick_timer_args = {};
    lvgl_tick_timer_args.callback = &example_increase_lvgl_tick;
    lvgl_tick_timer_args.name = "lvgl_tick";
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));

    // 8. 创建任务
    ESP_LOGI(TAG, "Step 11: Create tasks");
    lvgl_mux = xSemaphoreCreateMutex();
    if (lvgl_mux == NULL) {
        ESP_LOGE(TAG, "Failed to create LVGL mutex");
        return;
    }
    
    BaseType_t ret1 = xTaskCreatePinnedToCore(example_lvgl_port_task, "LVGL", LVGL_TASK_STACK_SIZE, NULL, LVGL_TASK_PRIORITY, NULL, 0);
    BaseType_t ret2 = xTaskCreatePinnedToCore(example_backlight_loop_task, "Backlight", 4 * 1024, NULL, 2, NULL, 0);
    
    if (ret1 != pdPASS || ret2 != pdPASS) {
        ESP_LOGE(TAG, "Failed to create tasks");
        return;
    }
    
    // 9. 显示初始界面
    ESP_LOGI(TAG, "Step 12: Show initial UI");
    if (example_lvgl_lock(-1)) {
        ui_update_current_mode(get_current_mode());
        example_lvgl_unlock();
    }
    
    ESP_LOGI(TAG, "=== System initialization completed ===");
    ESP_LOGI(TAG, "Current mode: %s", get_mode_name(get_current_mode()));
    ESP_LOGI(TAG, "Press BOOT button to switch modes, long press for network config");
    
    // 主循环 - 定期喂狗
    while (1) {
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}