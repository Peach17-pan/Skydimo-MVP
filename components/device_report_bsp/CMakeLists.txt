idf_component_register(
    SRCS "device_report_bsp.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi esp_netif esp_timer freertos lwip
    PRIV_REQUIRES json
)





