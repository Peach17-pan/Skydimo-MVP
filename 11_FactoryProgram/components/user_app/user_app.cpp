#include <stdio.h>
#include "user_app.h"
#include "lvgl.h"

#include "esp_timer.h"
#include "esp_log.h"
#include "button_bsp.h"
#include "esp_log.h"
#include "adc_bsp.h"
#include "esp_system.h"
#include "user_config.h"
#include "i2c_equipment.h"
#include "lcd_bl_pwm_bsp.h"
#include "esp_wifi_bsp.h"

#include "ble_scan_bsp.h"
#include <esp_io_expander_tca9554.h>
#include "user_audio_bsp.h"
#include "mode_manager.h" 

lv_ui src_ui;
esp_io_expander_handle_t io_expander = NULL;
QueueHandle_t app_touch_data_queue = NULL;
static EventGroupHandle_t app_touch_group;
static uint8_t *audio_ptr = NULL;
static EventGroupHandle_t lvgl_button_groups;
static bool is_vbatpowerflag = false;

// 新增：模式相关变量
static device_mode_t current_display_mode = MODE_CLOCK;
static QueueHandle_t mode_change_queue = NULL;

/*静态任务*/
static void example_color_task(void *arg);
static void example_button_boot_task(void* arg);
static void example_button_pwr_task(void* arg);
static void example_user_task(void* arg);
static void example_sdcard_task(void *arg);
static void example_scan_wifi_ble_task(void *arg);
static void example_test_touch_task(void *arg);
/*静态函数*/
static void tca9554_init(void);
static void set_canvas_color(int16_t x,int16_t y);
static void set_canvas_fill(void);
static void lvgl_obj_event_callback(lv_event_t *e);

// 新增：各个模式的UI更新函数
static void update_network_config_ui(lv_ui *ui);
static void update_clock_ui(lv_ui *ui);
static void update_weather_ui(lv_ui *ui);
static void update_gallery_ui(lv_ui *ui);
static void update_virtual_keyboard_ui(lv_ui *ui);

static void audio_Test_Callback(lv_event_t *e)
{
	assert(audio_ptr);
	lv_event_code_t code = lv_event_get_code(e);
  	lv_ui *ui = (lv_ui *)e->user_data;
  	lv_obj_t * module = e->current_target;
  	switch (code)
  	{
  	  case LV_EVENT_CLICKED:
  	  {
  	    if(module == ui->screen_btn_1)
  	    {
			xEventGroupSetBits(lvgl_button_groups,set_bit_button(0));
  	    }
		if(module == ui->screen_btn_2)
		{
			xEventGroupSetBits(lvgl_button_groups,set_bit_button(1));
		}
  	    break;
  	  }
  	  default:
  	    break;
  	}
}

void i2s_audio_Test(void *arg)
{
	lv_ui *ui = (lv_ui *)arg;
	for(;;)
	{
		EventBits_t even = xEventGroupWaitBits(lvgl_button_groups,set_bit_all,pdTRUE,pdFALSE,pdMS_TO_TICKS(8 * 1000));
		if(get_bit_button(even,0))
		{
			lv_label_set_text(ui->screen_label_24, "正在录音");
			lv_label_set_text(ui->screen_label_25, "Recording...");

			audio_playback_read(audio_ptr,192 * 1000);
			lv_label_set_text(ui->screen_label_24, "录音完成");
			lv_label_set_text(ui->screen_label_25, "Rec Done");
		}
		else if(get_bit_button(even,1))
		{
			lv_label_set_text(ui->screen_label_24, "正在播放");
			lv_label_set_text(ui->screen_label_25, "Playing...");
			audio_playback_write(audio_ptr,192 * 1000);
			lv_label_set_text(ui->screen_label_24, "播放完成");
			lv_label_set_text(ui->screen_label_25, "Play Done");
		}
		else
		{
			lv_label_set_text(ui->screen_label_24, "等待操作");
			lv_label_set_text(ui->screen_label_25, "Idle");
		}
	}
}

void power_Test(void *arg)
{
	if(gpio_get_level(GPIO_NUM_16))
	{
		is_vbatpowerflag = true;
	}
	vTaskDelete(NULL); 
}

void user_app_init(void)
{
	lvgl_button_groups = xEventGroupCreate();
	app_touch_group = xEventGroupCreate();
	app_touch_data_queue = xQueueCreate(10,sizeof(app_touch_t));
  	//audio_ptr = (uint8_t *)heap_caps_malloc(192 * 1000 * sizeof(uint8_t), MALLOC_CAP_SPIRAM);
	setup_ui(&src_ui);
  	lcd_bl_pwm_bsp_init(LCD_PWM_MODE_255);
	tca9554_init();
  	button_Init();
  	adc_bsp_init();
  	i2c_rtc_setup();
  	i2c_rtc_setTime(2025,7,7,18,43,30);
  	i2c_imu_setup();
  	_sdcard_init();
  	espwifi_init();
	//user_audio_bsp_init();
	//audio_play_init();

	// 新增：初始化模式管理器
    mode_manager_init();
    mode_change_queue = get_mode_change_queue();

  	xTaskCreatePinnedToCore(example_sdcard_task, "example_sdcard_task", 3 * 1024, &src_ui, 2, NULL,1);      //sd card测试
  	xTaskCreatePinnedToCore(example_user_task, "example_user_task", 4 * 1024, &src_ui, 2, NULL,1);     //用户事件
  	xTaskCreatePinnedToCore(example_color_task, "example_color_task", 4 * 1024, &src_ui, 2, NULL,1);   //RGB颜色测试
  	xTaskCreatePinnedToCore(example_button_boot_task, "example_button_boot_task", 4 * 1024, &src_ui, 2, NULL,1);    //按钮事件
  	xTaskCreatePinnedToCore(example_button_pwr_task, "example_button_pwr_task", 4 * 1024, &src_ui, 2, NULL,1);      //按钮事件  
  	xTaskCreatePinnedToCore(example_scan_wifi_ble_task, "example_scan_wifi_ble_task", 3 * 1024,&src_ui, 2, NULL,1);   
	xTaskCreatePinnedToCore(example_test_touch_task, "example_test_touch_task", 3 * 1024,&src_ui, 2, NULL,1);       //测试touch任务
	//xTaskCreatePinnedToCore(i2s_audio_Test, "i2s_audio_Test", 4 * 1024, &src_ui, 3, NULL,1);                           //audio        
	xTaskCreatePinnedToCore(power_Test, "power_Test", 4 * 1024, NULL, 3, NULL,1);                           //audio

    // 新增：创建模式管理任务
    xTaskCreatePinnedToCore(example_mode_manager_task, "example_mode_manager_task", 4 * 1024, &src_ui, 2, NULL,1);

	/*even add*/
  	lv_obj_add_event_cb(src_ui.screen_slider_1, lvgl_obj_event_callback, LV_EVENT_ALL, &src_ui); 
	//lv_obj_add_event_cb(src_ui.screen_btn_1, audio_Test_Callback, LV_EVENT_ALL, &src_ui); 
	//lv_obj_add_event_cb(src_ui.screen_btn_2, audio_Test_Callback, LV_EVENT_ALL, &src_ui);

	// 新增：初始化显示当前模式
    user_app_switch_mode(get_current_mode());
}

// 新增：模式管理任务
static void example_mode_manager_task(void *arg)
{
    lv_ui *ui = (lv_ui *)arg;
    mode_change_event_t mode_event;
    
    for(;;) {
        // 等待模式切换事件
        if (xQueueReceive(mode_change_queue, &mode_event, portMAX_DELAY) == pdTRUE) {
            // 更新UI显示
            user_app_switch_mode(mode_event.new_mode);
        }
    }
}

// 新增：模式切换函数
void user_app_switch_mode(device_mode_t mode)
{
    if (current_display_mode == mode) {
        return; // 已经是当前模式，无需切换
    }
    
    current_display_mode = mode;
    
    // 根据模式更新UI显示
    switch (mode) {
        case MODE_NETWORK_CONFIG:
            update_network_config_ui(&src_ui);
            break;
        case MODE_CLOCK:
            update_clock_ui(&src_ui);
            break;
        case MODE_WEATHER:
            update_weather_ui(&src_ui);
            break;
        case MODE_GALLERY:
            update_gallery_ui(&src_ui);
            break;
        case MODE_VIRTUAL_KEYBOARD:
            update_virtual_keyboard_ui(&src_ui);
            break;
        default:
            break;
    }
    
    ESP_LOGI("ModeManager", "UI switched to %s", MODE_NAMES[mode]);
}

// 新增：获取当前模式
device_mode_t user_app_get_current_mode(void)
{
    return current_display_mode;
}

// 新增：各个模式的UI更新实现
static void update_network_config_ui(lv_ui *ui)
{
    // 显示配网模式相关信息
    lv_label_set_text(ui->screen_label_21, "配网模式 - 等待配置");
    // 可以在这里显示网络状态、IP地址等信息
}

static void update_clock_ui(lv_ui *ui)
{
    // 显示时钟模式 - 保持原有的RTC显示
    lv_label_set_text(ui->screen_label_21, "时钟模式 - 显示时间");
}

static void update_weather_ui(lv_ui *ui)
{
    // 显示天气模式
    lv_label_set_text(ui->screen_label_21, "天气模式 - 获取天气中...");
}

static void update_gallery_ui(lv_ui *ui)
{
    // 显示相册模式
    lv_label_set_text(ui->screen_label_21, "相册模式 - 准备显示图片");
}

static void update_virtual_keyboard_ui(lv_ui *ui)
{
    // 显示虚拟键盘模式
    lv_label_set_text(ui->screen_label_21, "虚拟键盘模式 - 等待配置");
}


static void example_test_touch_task(void *arg)
{
	for(;;)
	{
		app_touch_t touch_data;
		EventBits_t even = xEventGroupWaitBits(app_touch_group,set_bit_all,pdFALSE,pdFALSE,pdMS_TO_TICKS(1000));
		if(get_bit_button(even,0))
		{
			if(pdTRUE == xQueueReceive(app_touch_data_queue,&touch_data,pdMS_TO_TICKS(1200)))
      		{
      		    set_canvas_color(touch_data.x,touch_data.y);
      		}
      		else
      		{
      		  	set_canvas_fill();
      		}
		}
		if(get_bit_button(even,1))
		{
			xEventGroupClearBits(app_touch_group,set_bit_all);
		}
	}
}

void example_scan_wifi_ble_task(void *arg)
{
  	lv_ui *Send_ui = (lv_ui *)arg;
  	char send_lvgl[10] = {""};
  	EventBits_t even = xEventGroupWaitBits(wifi_even_,0x02,pdTRUE,pdTRUE,pdMS_TO_TICKS(30000)); 
  	espwifi_deinit(); //释放WIFI
  	ble_scan_prepare();
  	ble_stack_init();
  	ble_scan_start();
  	if(get_bit_button(even,1))
  	{
  	  	snprintf(send_lvgl,9,"%d",user_esp_bsp.apNum);
  	  	lv_label_set_text(Send_ui->screen_label_19, send_lvgl);
  	}
  	else
  	{
  	  	lv_label_set_text(Send_ui->screen_label_19, "P");
  	}
  	even = xEventGroupWaitBits(ble_even_,0x01,pdTRUE,pdTRUE,pdMS_TO_TICKS(30000));
  	if(get_bit_button(even,0))
  	{
		snprintf(send_lvgl,9,"%d",ble_scan_apNum);
		lv_label_set_text(Send_ui->screen_label_18, send_lvgl);
  	}
  	else
  	{
  	  	lv_label_set_text(Send_ui->screen_label_18, "P");
  	}
	ESP_LOGE("scan","(%d,%d)",ble_scan_apNum,user_esp_bsp.apNum);
  	ble_stack_deinit();     //释放BLE
  	vTaskDelete(NULL);
}

static void example_sdcard_task(void *arg)
{
  	lv_ui *ui = (lv_ui *)arg;
  	char lvgl_send_data[10] = {""};
  	EventBits_t even = xEventGroupWaitBits(sdcard_even_,(0x01),pdTRUE,pdFALSE,pdMS_TO_TICKS(8 * 1000));
  	if(get_bit_button(even,0))
  	{
  	  	snprintf(lvgl_send_data,10,"%.2fG",user_sdcard_bsp.sdcard_size);
  	  	lv_label_set_text(ui->screen_label_9, lvgl_send_data);
  	}
  	else
  	{
  	  	lv_label_set_text(ui->screen_label_9, "NULL");
  	}
  	vTaskDelay(pdMS_TO_TICKS(1000));
  	vTaskDelete(NULL); 
}

static void example_user_task(void* arg)
{
  	lv_ui *ui = (lv_ui *)arg;

  	uint32_t times = 0;
  	uint32_t rtc_time = 0;
  	uint32_t imu_time = 0;
  	uint32_t adc_time = 0;

  	float adc_value = 0;
  	char lvgl_send_data[30] = {""};
  	for(;;)
  	{
  	  	if(times - adc_time == 10) //2s
  	  	{
			adc_time = times;
			adc_get_value(&adc_value,NULL);
			snprintf(lvgl_send_data,28,"%.2fV",adc_value);
			lv_label_set_text(ui->screen_label_7, lvgl_send_data);
  	  	}
  	  	if(times - rtc_time == 5) //1s
  	  	{
  	  	  	rtc_time = times;
  	  	  	RtcDateTime_t rtc = i2c_rtc_get();
  	  	  	snprintf(lvgl_send_data,28,"%d/%02d/%02d",rtc.year,rtc.month,rtc.day);
  	  	  	lv_label_set_text(ui->screen_label_11, lvgl_send_data);
  	  	  	snprintf(lvgl_send_data,28,"%02d:%02d:%02d",rtc.hour,rtc.minute,rtc.second);
  	  	  	lv_label_set_text(ui->screen_label_12, lvgl_send_data);
  	  	}
  	  	if(times - imu_time == 1) //200ms
  	  	{
  	  	  	imu_time = times;
  	  	  	ImuDate_t imuData = i2c_imu_get();
  	  	  	snprintf(lvgl_send_data,28,"%.2f,%.2f,%.2f (g)",imuData.accx,imuData.accy,imuData.accz);
  	  	  	lv_label_set_text(ui->screen_label_14, lvgl_send_data);
  	  	  	snprintf(lvgl_send_data,28,"%.2f,%.2f,%.2f (dps)",imuData.gyrox,imuData.gyroy,imuData.gyroz);
  	  	  	lv_label_set_text(ui->screen_label_15, lvgl_send_data);
  	  	}
  	  	vTaskDelay(pdMS_TO_TICKS(200));
  	  	times++;
  	}
}

static void example_color_task(void *arg)
{
  	lv_ui *ui = (lv_ui *)arg;
  	lv_obj_clear_flag(ui->screen_carousel_1,LV_OBJ_FLAG_SCROLLABLE); //unmovable

  	lv_obj_clear_flag(ui->screen_cont_1,LV_OBJ_FLAG_HIDDEN); 
  	lv_obj_add_flag(ui->screen_cont_2, LV_OBJ_FLAG_HIDDEN);
  	lv_obj_add_flag(ui->screen_cont_3, LV_OBJ_FLAG_HIDDEN);

  	lv_obj_clear_flag(ui->screen_img_1,LV_OBJ_FLAG_HIDDEN); 
  	lv_obj_add_flag(ui->screen_img_2, LV_OBJ_FLAG_HIDDEN);
  	lv_obj_add_flag(ui->screen_img_3, LV_OBJ_FLAG_HIDDEN);
  	vTaskDelay(pdMS_TO_TICKS(1500));
  	lv_obj_clear_flag(ui->screen_img_2,LV_OBJ_FLAG_HIDDEN); 
  	lv_obj_add_flag(ui->screen_img_1, LV_OBJ_FLAG_HIDDEN);
  	lv_obj_add_flag(ui->screen_img_3, LV_OBJ_FLAG_HIDDEN);
  	vTaskDelay(pdMS_TO_TICKS(1500));
  	lv_obj_clear_flag(ui->screen_img_3,LV_OBJ_FLAG_HIDDEN); 
  	lv_obj_add_flag(ui->screen_img_2, LV_OBJ_FLAG_HIDDEN);
  	lv_obj_add_flag(ui->screen_img_1, LV_OBJ_FLAG_HIDDEN);
  	vTaskDelay(pdMS_TO_TICKS(1500));

  	lv_obj_clear_flag(ui->screen_cont_2,LV_OBJ_FLAG_HIDDEN); 
  	lv_obj_add_flag(ui->screen_cont_1, LV_OBJ_FLAG_HIDDEN);
  	lv_obj_add_flag(ui->screen_cont_3, LV_OBJ_FLAG_HIDDEN);

  	lv_obj_add_flag(ui->screen_carousel_1,LV_OBJ_FLAG_SCROLLABLE); //removable
  	vTaskDelete(NULL); 
}

static void tca9554_init(void)
{
  	i2c_master_bus_handle_t tca9554_i2c_bus_ = NULL;
  	ESP_ERROR_CHECK(i2c_master_get_bus_handle(0,&tca9554_i2c_bus_));
  	esp_io_expander_new_i2c_tca9554(tca9554_i2c_bus_, ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_000, &io_expander);
	esp_io_expander_set_dir(io_expander, IO_EXPANDER_PIN_NUM_7 | IO_EXPANDER_PIN_NUM_6, IO_EXPANDER_OUTPUT);
  	esp_io_expander_set_level(io_expander, IO_EXPANDER_PIN_NUM_7 | IO_EXPANDER_PIN_NUM_6, 1);
}

// 修改：example_button_boot_task - 集成模式切换
static void example_button_boot_task(void* arg)
{
    lv_ui *ui = (lv_ui *)arg;
    uint32_t sdcard_test = 0;
    bool is_touchflag = true;
    bool is_audioflag = false;
    char sdcard_buf[35] = {""};
    char sdcard_rec[35] = {""};
    
    for (;;)
    {
        EventBits_t even = xEventGroupWaitBits(boot_groups,set_bit_all,pdTRUE,pdFALSE,pdMS_TO_TICKS(2 * 1000));
        
        if(get_bit_button(even,0)) // 短按 - 切换模式
        {
            switch_to_next_mode();
        }
        else if(get_bit_button(even,1)) // 双击 - 保持原有SD卡测试功能
        {
            sdcard_test++;
            snprintf(sdcard_buf,33,"sdcardTest : %ld",sdcard_test);
            sdcard_file_write("/sdcard/sdcardTest.txt",sdcard_buf);
            sdcard_file_read("/sdcard/sdcardTest.txt",sdcard_rec,NULL);
            if(!strcmp(sdcard_rec,sdcard_buf))
            {
                lv_label_set_text(ui->screen_label_21, "sdcard test passed");
            }
            else
            {
                lv_label_set_text(ui->screen_label_21, "sdcard test failed");
            }
        }
        else if(get_bit_button(even,2)) // 长按 - 直接进入配网模式
        {
            switch_to_mode(MODE_NETWORK_CONFIG);
        }
        else if(get_bit_button(even,3)) // 弹起
        {
            // 保持原有功能或不做处理
        }
        else
        {
            // 显示当前模式信息
            lv_label_set_text(ui->screen_label_21, MODE_NAMES[user_app_get_current_mode()]);
        }
    }
}

static void example_button_pwr_task(void* arg)
{
  	lv_ui *ui = (lv_ui *)arg;
  	for (;;)
  	{
  	  	EventBits_t even = xEventGroupWaitBits(pwr_groups,set_bit_all,pdTRUE,pdFALSE,pdMS_TO_TICKS(2 * 1000));
  	  	if(get_bit_button(even,0))
  	  	{

  	  	}
  	  	else if(get_bit_button(even,1)) //长按
  	  	{
			if(is_vbatpowerflag)
			{
				is_vbatpowerflag = false;
				esp_io_expander_set_level(io_expander, IO_EXPANDER_PIN_NUM_6, 0);
			}
  	  	}
  	  	else if(get_bit_button(even,2))
  	  	{
			if(!is_vbatpowerflag)
			{
				is_vbatpowerflag = true;
			}
  	  	}
  	}
}

static void set_canvas_color(int16_t x,int16_t y)
{
  lv_color_t color = lv_palette_main(LV_PALETTE_ORANGE);
  lv_canvas_set_px_color(src_ui.screen_canvas_1, x, y, color);
}

static void set_canvas_fill(void)
{
  lv_color_t bg_color = lv_color_black();    //黑色
  lv_canvas_fill_bg(src_ui.screen_canvas_1, bg_color, LV_OPA_COVER);
}

static void lvgl_obj_event_callback(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
  	lv_ui *ui = (lv_ui *)e->user_data;
  	lv_obj_t * module = e->current_target;
  	switch (code)
  	{
  	  case LV_EVENT_CLICKED:
  	  {
  	    if(module == ui->screen_slider_1)
  	    {
  	      uint8_t value = lv_slider_get_value(module);
  	      setUpduty(0xff - value);
  	    }
  	    break;
  	  }
  	  default:
  	    break;
  	}
}


