#include "lvgl.h"
#include "lcd_driver.h"

/*********************
 *      DEFINES
 *********************/
#define EXAMPLE_LCD_H_RES 320
#define EXAMPLE_LCD_V_RES 480

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_display_t * display;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_disp_init(void)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    
    /* Initialize LCD driver */
    lcd_driver_init();

    /*-----------------------------------
     * Create a display and set a flush_cb
     *----------------------------------*/
    display = lv_display_create(EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    lv_display_set_flush_cb(display, disp_flush);

    /* Example 1: Use 2 buffers */
    static lv_color_t buf1[EXAMPLE_LCD_H_RES * 80];
    static lv_color_t buf2[EXAMPLE_LCD_H_RES * 80];
    lv_display_set_buffers(display, buf1, buf2, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    /* Example 2: Use 1 buffer */
    // static lv_color_t buf1[EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES];
    // lv_display_set_buffers(display, buf1, NULL, sizeof(buf1), LV_DISPLAY_RENDER_MODE_FULL);

    /* Set rotation */
    lv_display_set_rotation(display, LV_DISPLAY_ROTATION_90);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    /* Set the display window */
    lcd_set_window(area->x1, area->y1, area->x2, area->y2);
    
    /* Send the pixel data to the display */
    lcd_send_data(px_map, ((area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1)) * 2);
    
    /* IMPORTANT!!!
     * Inform the graphics library that you are ready with the flushing */
    lv_display_flush_ready(disp);
}