#include "i2c_bsp.h"
#include "user_config.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "I2C_BSP";
i2c_master_bus_handle_t user_i2c_port1_handle = NULL;
i2c_master_dev_handle_t disp_touch_dev_handle = NULL;

static uint32_t i2c_data_timeout = 0;
static uint32_t i2c_done_timeout = 0;

// 修复函数名：touch_i2c_master_Init（注意大小写）
void touch_i2c_master_Init(void) {
    i2c_data_timeout = pdMS_TO_TICKS(5000);
    i2c_done_timeout = pdMS_TO_TICKS(1000);

    ESP_LOGI(TAG, "初始化触摸I2C总线...");
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_1,
        .scl_io_num = Touch_SCL_NUM,
        .sda_io_num = Touch_SDA_NUM,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &user_i2c_port1_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .scl_speed_hz = 300000,
        .device_address = DISP_TOUCH_ADDR,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(user_i2c_port1_handle, &dev_cfg, &disp_touch_dev_handle));
    
    ESP_LOGI(TAG, "触摸I2C初始化完成");
}

// 修复函数名：i2c_write_buff（注意拼写）
uint8_t i2c_write_buff(i2c_master_dev_handle_t dev_handle, int reg, uint8_t *buf, uint8_t len) {
    uint8_t ret = i2c_master_bus_wait_all_done(user_i2c_port1_handle, i2c_done_timeout);
    if (ret != ESP_OK) return ret;
    
    if (reg == -1) {
        return i2c_master_transmit(dev_handle, buf, len, i2c_data_timeout);
    } else {
        uint8_t *pbuf = (uint8_t *)malloc(len + 1);
        pbuf[0] = reg;
        memcpy(&pbuf[1], buf, len);
        ret = i2c_master_transmit(dev_handle, pbuf, len + 1, i2c_data_timeout);
        free(pbuf);
        return ret;
    }
}

uint8_t i2c_read_buff(i2c_master_dev_handle_t dev_handle, int reg, uint8_t *buf, uint8_t len) {
    uint8_t ret = i2c_master_bus_wait_all_done(user_i2c_port1_handle, i2c_done_timeout);
    if (ret != ESP_OK) return ret;
    
    if (reg == -1) {
        return i2c_master_receive(dev_handle, buf, len, i2c_data_timeout);
    } else {
        uint8_t addr = (uint8_t)reg;
        return i2c_master_transmit_receive(dev_handle, &addr, 1, buf, len, i2c_data_timeout);
    }
}

// 添加缺失的函数实现
uint8_t i2c_master_write_read_dev(i2c_master_dev_handle_t dev_handle, uint8_t *writeBuf, uint8_t writeLen, uint8_t *readBuf, uint8_t readLen) {
    uint8_t ret = i2c_master_bus_wait_all_done(user_i2c_port1_handle, i2c_done_timeout);
    if (ret != ESP_OK) return ret;
    
    return i2c_master_transmit_receive(dev_handle, writeBuf, writeLen, readBuf, readLen, i2c_data_timeout);
}