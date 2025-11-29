idf_component_register(
    SRCS "captive_portal_bsp.c"
    INCLUDE_DIRS "."
    REQUIRES esp_http_server esp_wifi nvs_flash esp_netif esp_event
    PRIV_REQUIRES json
)

