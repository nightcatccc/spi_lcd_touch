idf_component_register(SRCS "zhiwen.c" "people.c" "wifi_on.c" "wifi_off.c" "kt_on.c" "kt_off.c" "light_on.c" "light_off.c" "setting.c" "lv_app.c" "error.c" "call.c" "timing.c" "bg.c" "light_on.c" "light_off.c" "kt_on.c" "kt_off.c"
                    "bg_on.c" "bg_off.c" "wifi_on.c" "wifi_off.c" "people.c" "zhiwen.c" "lose.c" "bad.c"
                    REQUIRES lvgl AS608 esp_wifi cJSON esp_http_client esp_https_ota esp_partition
                    INCLUDE_DIRS "include")
