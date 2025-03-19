idf_component_register(SRCS "spi_lcd_touch_example_main.c" "lvgl_demo_ui.c"
                       REQUIRES lv_app esp_wifi nvs_flash cJSON esp_http_client mbedtls
                       INCLUDE_DIRS ".")
