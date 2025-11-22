#include "user_config.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_lcd_panel_ops.h"
#include "lvgl.h"

// 添加断言支持
#include <assert.h>

/*********************
 *      DEFINES
 *********************/
#define TAG "LV_DISP"

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1;
static lv_color_t *buf2;
static spi_device_handle_t spi;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_disp_init(void)
{
    ESP_LOGI(TAG, "Initializing display port");
    
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();

    /*-----------------------------
     * Create a buffer for drawing
     *----------------------------*/
    
    /* LVGL requires a buffer where it draws the objects. 
     * Later this buffer will passed to your display driver to flush the content */
    
    /* Example for 1/10 screen size buffer */
    uint32_t buf_size = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES / 10;
    
    /* Allocate memory */
    buf1 = heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1);
    
    buf2 = heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2);
    
    /* Initialize `disp_buf` with the buffer(s) */
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buf_size);

    /*-----------------------------------
     * Register the display in LVGL
     *----------------------------------*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);

    /*Set the resolution of the display*/
    disp_drv.hor_res = EXAMPLE_LCD_H_RES;
    disp_drv.ver_res = EXAMPLE_LCD_V_RES;

    /*Used to copy the buffer's content to the display*/
    disp_drv.flush_cb = disp_flush;

    /*Set a display buffer*/
    disp_drv.draw_buf = &draw_buf;

    /*Required for Example 3)*/
    //disp_drv.full_refresh = 1;

    /* Fill a memory array with a color if you have GPU.
     * Note that, in lv_conf.h you can enable GPUs that has built-in support in LVGL.
     * But if you have a different GPU you can use with this callback.*/
    //disp_drv.gpu_fill_cb = gpu_fill;

    /*Finally register the driver*/
    lv_disp_t * disp = lv_disp_drv_register(&disp_drv);
    
    ESP_LOGI(TAG, "Display port initialized");
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/* Initialize your display and the required peripherals. */
static void disp_init(void)
{
    ESP_LOGI(TAG, "Initializing display hardware");
    
    // 这里需要根据你的具体屏幕型号实现SPI初始化
    // 以下是示例代码，需要根据实际硬件调整
    
    spi_bus_config_t buscfg = {
        .miso_io_num = -1,
        .mosi_io_num = EXAMPLE_PIN_NUM_LCD_DATA0,
        .sclk_io_num = EXAMPLE_PIN_NUM_LCD_PCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * 2 + 8
    };
    
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000,  // Clock out at 10 MHz
        .mode = 0,                           // SPI mode 0
        .spics_io_num = EXAMPLE_PIN_NUM_LCD_CS,
        .queue_size = 7,
        .flags = SPI_DEVICE_NO_DUMMY,
    };
    
    // Initialize the SPI bus
    ret = spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    
    // Attach the LCD to the SPI bus
    ret = spi_bus_add_device(LCD_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);
    
    // 初始化GPIO引脚
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << EXAMPLE_PIN_NUM_LCD_RST) | (1ULL << EXAMPLE_PIN_NUM_BK_LIGHT),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    
    // 复位屏幕
    gpio_set_level(EXAMPLE_PIN_NUM_LCD_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(EXAMPLE_PIN_NUM_LCD_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // 打开背光
    gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, 1);
    
    ESP_LOGI(TAG, "Display hardware initialized");
}

/* Flush the content of the internal buffer the specific area on the display
 * You can use DMA or any hardware acceleration to do this operation in the background but
 * 'lv_disp_flush_ready()' has to be called when finished. */
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    ESP_LOGD(TAG, "Flush display: (%d,%d) to (%d,%d)", 
             area->x1, area->y1, area->x2, area->y2);
    
    // 这里需要实现具体的屏幕刷新函数
    // 根据你的屏幕驱动芯片来编写
    
    /* IMPORTANT!!!
     * Inform the graphics library that you are ready with the flushing*/
    lv_disp_flush_ready(disp_drv);
}