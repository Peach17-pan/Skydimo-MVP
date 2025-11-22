#include "lv_port_indev.h"
#include "user_config.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "driver/gpio.h"

/*********************
 *      DEFINES
 *********************/
#define TAG "LV_INDEV"

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void touchpad_init(void);
static void touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_indev_t * indev_touchpad;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_indev_init(void)
{
    ESP_LOGI(TAG, "Initializing input device port");

    /*------------------
     * Touchpad
     * -----------------*/
    
    /*Initialize your touchpad if you have*/
    touchpad_init();

    /*Register a touchpad input device*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    indev_touchpad = lv_indev_drv_register(&indev_drv);
    
    ESP_LOGI(TAG, "Input device port initialized");
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/* Initialize your touchpad */
static void touchpad_init(void)
{
    ESP_LOGI(TAG, "Initializing touchpad");
    
    // 初始化I2C用于触摸屏
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = Touch_SDA_NUM,
        .scl_io_num = Touch_SCL_NUM,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    
    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
    
    ESP_LOGI(TAG, "Touchpad initialized");
}

/* Will be called by the library to read the touchpad */
static void touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    static lv_coord_t last_x = 0;
    static lv_coord_t last_y = 0;

    /*Save the pressed coordinates and the state*/
    // 这里需要实现具体的触摸读取函数
    // 根据你的触摸芯片来编写
    
    bool touched = false; // 需要读取实际的触摸状态
    lv_coord_t x = 0;     // 需要读取实际的X坐标
    lv_coord_t y = 0;     // 需要读取实际的Y坐标
    
    if(touched) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = x;
        data->point.y = y;
        last_x = data->point.x;
        last_y = data->point.y;
        ESP_LOGD(TAG, "Touch at (%d,%d)", x, y);
    } else {
        data->state = LV_INDEV_STATE_REL;
        data->point.x = last_x;
        data->point.y = last_y;
    }
}