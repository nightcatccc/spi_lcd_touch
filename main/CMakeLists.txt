idf_component_register(SRCS "spi_lcd_touch_example_main.c" "lvgl_demo_ui.c"
                       REQUIRES lv_app esp_wifi nvs_flash
                       INCLUDE_DIRS ".")
                       set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/components/esp_wifi)