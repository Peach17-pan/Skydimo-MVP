#include "lv_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void lv_port_task(void)
{
    lv_tick_inc(5); // 5ms tick
    lv_timer_handler(); // Call LVGL timer handler
}