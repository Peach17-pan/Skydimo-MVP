idf_component_register(
    SRCS "button_bsp.c" "multi_button.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_timer
)

