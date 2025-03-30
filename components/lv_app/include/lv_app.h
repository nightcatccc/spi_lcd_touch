#ifndef LV_APP_H
#define LV_APP_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
extern const lv_img_dsc_t error;
extern const lv_img_dsc_t setting ;
extern const lv_img_dsc_t call;
extern const lv_img_dsc_t timing;
extern const lv_img_dsc_t light_on;
extern const lv_img_dsc_t light_off;
extern const lv_img_dsc_t kt_on;
extern const lv_img_dsc_t kt_off;
extern const lv_img_dsc_t bg_on;
extern const lv_img_dsc_t bg_off;
extern const lv_img_dsc_t people;
extern const lv_img_dsc_t zhiwen;
//extern const lv_img_dsc_t wifi_on;
//extern const lv_img_dsc_t wifi_off;

void app_text(void);
#endif