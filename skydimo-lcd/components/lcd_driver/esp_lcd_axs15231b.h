#ifndef ESP_LCD_AXS15231B_H
#define ESP_LCD_AXS15231B_H

#include "esp_lcd_panel_vendor.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LCD panel initialization commands.
 */
typedef struct {
    uint8_t cmd;
    uint8_t *data;
    uint16_t data_bytes;
    uint16_t delay_ms;
} axs15231b_lcd_init_cmd_t;

/**
 * @brief Vendor specific configuration
 */
typedef struct {
    const axs15231b_lcd_init_cmd_t *init_cmds;
    uint16_t init_cmds_size;
    struct {
        uint32_t use_qspi_interface: 1;
    } flags;
} axs15231b_vendor_config_t;

/**
 * @brief Create LCD panel for model axs15231b
 *
 * @param[in] io LCD panel IO handle
 * @param[in] panel_dev_config general panel device configuration
 * @param[out] ret_panel Returned LCD panel handle
 * @return
 *          - ESP_ERR_INVALID_ARG   if parameter is invalid
 *          - ESP_ERR_NO_MEM        if out of memory
 *          - ESP_OK                on success
 */
esp_err_t esp_lcd_new_panel_axs15231b(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel);

#ifdef __cplusplus
}
#endif

#endif /* ESP_LCD_AXS15231B_H */