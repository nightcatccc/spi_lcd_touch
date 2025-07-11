/*
 * SPDX-FileCopyrightText: 2021-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */


#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "lv_demos.h"
#include "time.h"
#include "stdlib.h"
#include "esp_wifi.h"
#include "driver/i2c.h"
#include "lv_app.h"
#include "lwip/apps/sntp.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "cJSON.h"
#include "AS608.h"
//#include "bsp_wifi_station.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"

extern int8_t base_1_flag;//主界面标志位
extern int8_t slider_flag;//设置按钮标志位
extern int8_t base_3_flag;//修理人员面板标志位
extern int8_t base_4_flag;//课表面板标志位

extern int8_t light_flag;//灯光标志位
extern int8_t kongtiao_flag;//空调标志位位
extern int8_t touying_flag;//投影仪标志位
#if CONFIG_EXAMPLE_LCD_CONTROLLER_ILI9341
#include "esp_lcd_st7796.h"
#elif CONFIG_EXAMPLE_LCD_CONTROLLER_GC9A01
#include "esp_lcd_gc9a01.h"
#endif

#if CONFIG_EXAMPLE_LCD_TOUCH_CONTROLLER_STMPE610
#include "esp_lcd_touch_ft5x06.h"
#endif

static const char *TAG = "example";

// Using SPI2 in the example
#define LCD_HOST  SPI2_HOST

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ     (80 * 1000 * 1000)
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  1
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_SCLK           14
#define EXAMPLE_PIN_NUM_MOSI           15
#define EXAMPLE_PIN_NUM_MISO           7
#define EXAMPLE_PIN_NUM_LCD_DC         16
#define EXAMPLE_PIN_NUM_LCD_RST        17
#define EXAMPLE_PIN_NUM_LCD_CS         18
#define EXAMPLE_PIN_NUM_BK_LIGHT       13
//#define EXAMPLE_PIN_NUM_TOUCH_CS       15

#define EXAMPLE_PIN_NUM_TOUCH_SDA 12 // I2C数据引脚
#define EXAMPLE_PIN_NUM_TOUCH_SCL 8 // I2C时钟引脚
#define TOUCH_FT6336_RST 9 // 复位引脚（若无则设为-1）
#define EXAMPLE_PIN_NUM_TOUCH_INT -1 // 中断引脚

// The pixel number in horizontal and vertical
#if CONFIG_EXAMPLE_LCD_CONTROLLER_ILI9341
#define EXAMPLE_LCD_H_RES              320
#define EXAMPLE_LCD_V_RES              480
#elif CONFIG_EXAMPLE_LCD_CONTROLLER_GC9A01
#define EXAMPLE_LCD_H_RES              240
#define EXAMPLE_LCD_V_RES              240
#endif
// Bit number used to represent command and parameter
#define EXAMPLE_LCD_CMD_BITS           8
#define EXAMPLE_LCD_PARAM_BITS         8
                                                                                                                                             
#define EXAMPLE_LVGL_TICK_PERIOD_MS    2
#define EXAMPLE_LVGL_TASK_MAX_DELAY_MS 500
#define EXAMPLE_LVGL_TASK_MIN_DELAY_MS 1
#define EXAMPLE_LVGL_TASK_STACK_SIZE   (4 * 1024)
#define EXAMPLE_LVGL_TASK_PRIORITY     2

#define CONFIG_WIFI_SSID "xiaomi13"
#define CONFIG_WIFI_PASSWORD "cyy123123"

static SemaphoreHandle_t lvgl_mux = NULL;

#if CONFIG_EXAMPLE_LCD_TOUCH_ENABLED
esp_lcd_touch_handle_t tp = NULL;
#endif

extern void example_lvgl_demo_ui(lv_disp_t *disp);

static void esp_initialize_sntp(void)
{
    //sntp_setoperatingmode(SNTP_OPMODE_POLL);
    //ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setservername(0, "ntp1.aliyun.com");
    
    
    sntp_init();
}
//extern struct tm timeinfo;
struct tm esp_wait_sntp_sync(void)
{
    char strftime_buf[64];
    esp_initialize_sntp();
    struct tm timeinfo = { 0 };
    // wait for time to be set
    time_t now = 0;

    int retry = 0;

    while (timeinfo.tm_year < (2019 - 1900)) {
        ESP_LOGD(TAG, "Waiting for system time to be set... (%d)", ++retry);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    // set timezone to China Standard Time
    setenv("TZ", "CST-8", 1);
    tzset();

    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    //ESP_LOGI(TAG, "The current date/time in Shanghai is: %s", strftime_buf);
    return timeinfo;
}
 
static bool example_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}

static void example_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    // copy a buffer's content to a specific area of the display
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
}

/* Rotate display and touch, when rotated screen in LVGL. Called when driver parameters are updated. */
static void example_lvgl_port_update_callback(lv_disp_drv_t *drv)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;

    switch (drv->rotated) {
        case LV_DISP_ROT_NONE:
        // Rotate LCD display
        //esp_lcd_panel_swap_xy(panel_handle, false);
        //esp_lcd_panel_mirror(panel_handle, false, true);
        //esp_lcd_panel_swap_xy(panel_handle, true);
        //esp_lcd_panel_mirror(panel_handle, true, true);
        esp_lcd_panel_swap_xy(panel_handle, false);
        esp_lcd_panel_mirror(panel_handle, true, false);
#if CONFIG_EXAMPLE_LCD_TOUCH_ENABLED
        // Rotate LCD touch
        esp_lcd_touch_set_mirror_y(tp, false);
        esp_lcd_touch_set_mirror_x(tp, false);
#endif
        break;
    case LV_DISP_ROT_90:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, true);
        esp_lcd_panel_mirror(panel_handle, true, true);
#if CONFIG_EXAMPLE_LCD_TOUCH_ENABLED
        // Rotate LCD touch
        esp_lcd_touch_set_mirror_y(tp, false);
        esp_lcd_touch_set_mirror_x(tp, false);
#endif
        break;
    case LV_DISP_ROT_180:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, false);
        esp_lcd_panel_mirror(panel_handle, false, true);
#if CONFIG_EXAMPLE_LCD_TOUCH_ENABLED
        // Rotate LCD touch
        esp_lcd_touch_set_mirror_y(tp, false);
        esp_lcd_touch_set_mirror_x(tp, false);
#endif
        break;
    case LV_DISP_ROT_270:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, true);
        esp_lcd_panel_mirror(panel_handle, false, false);
#if CONFIG_EXAMPLE_LCD_TOUCH_ENABLED
        // Rotate LCD touch
        esp_lcd_touch_set_mirror_y(tp, false);
        esp_lcd_touch_set_mirror_x(tp, false);
#endif
        break;

    }
}

#if CONFIG_EXAMPLE_LCD_TOUCH_ENABLED
static void example_lvgl_touch_cb(lv_indev_drv_t * drv, lv_indev_data_t * data)
{
    uint16_t touchpad_x[1] = {0};
    uint16_t touchpad_y[1] = {0};
    uint8_t touchpad_cnt = 0;
    esp_lcd_touch_read_data(drv->user_data);
    bool touchpad_pressed = esp_lcd_touch_get_coordinates(drv->user_data, touchpad_x, touchpad_y, NULL, &touchpad_cnt, 1);

    if (touchpad_pressed && touchpad_cnt > 0) {
        data->point.x = touchpad_x[0];
        data->point.y = touchpad_y[0];
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
#endif

static void example_increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

bool example_lvgl_lock(int timeout_ms)
{
    // Convert timeout in milliseconds to FreeRTOS ticks
    // If `timeout_ms` is set to -1, the program will block until the condition is met
    const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks) == pdTRUE;
}

void example_lvgl_unlock(void)
{
    xSemaphoreGiveRecursive(lvgl_mux);
}

extern lv_obj_t * label_base_1_2;
static const char *TAG_http = "http";
void http_test_task(void *pvParameters) {
    char output_buffer[2048]={0};//缓冲区
    int content_length=0;//协议头长度

    esp_http_client_config_t config;
    memset(&config,0,sizeof(config));//定义、初始化结构体

    static const char* URL = "https://api.seniverse.com/v3/weather/now.json?key=SaBQu8ubUrQ-6YEOv&location=guilin&language=en&unit=c";
    config.url=URL;
    config.crt_bundle_attach=esp_crt_bundle_attach;
    //config.crt_bundle_attach = esp_crt_bundle_attach; // 使用证书包验证服务器

    esp_http_client_handle_t client =esp_http_client_init(&config);//初始化http

    esp_http_client_set_method(client,HTTP_METHOD_GET);
    while(1){
        esp_err_t err =esp_http_client_open(client,0);
       if(err!=ESP_OK){
            printf("open error");

        }
        else{
        content_length = esp_http_client_fetch_headers(client);

        if(content_length < 0){
            ESP_LOGE(TAG_http,"recive error");
        }
        else{
            int data_read = esp_http_client_read_response(client,output_buffer,2048);
            if(data_read > 0){
                ESP_LOGE(TAG_http,"HTTP GET STATUS : %d , content_length = %d",(int)esp_http_client_get_status_code(client),(int)esp_http_client_get_content_length(client));
                printf("DATA:%s\n",output_buffer);

                cJSON * root_data = NULL;
                root_data = cJSON_Parse(output_buffer);

                cJSON* cjson_item =cJSON_GetObjectItem(root_data,"results");
                cJSON* cjson_results =  cJSON_GetArrayItem(cjson_item,0);
                cJSON* cjson_now = cJSON_GetObjectItem(cjson_results,"now");
                cJSON* cjson_text = cJSON_GetObjectItem(cjson_now,"text");
                
                //printf("%d\n",cjson_text->type);
                //printf("%s\n",cjson_text->valuestring);
                lv_label_set_text_fmt(label_base_1_2, "Recent weather : %s", cjson_text->valuestring);
                cJSON_Delete(root_data);
                }
            }
        }
        esp_http_client_close(client);
        vTaskDelay(6000/portTICK_PERIOD_MS);
    

    }
}
//extern lv_obj_t * base_1_2;
static void example_lvgl_port_task(void *arg)
{
    ESP_LOGI(TAG, "Starting LVGL task");
    uint32_t task_delay_ms = EXAMPLE_LVGL_TASK_MAX_DELAY_MS;
    while (1) {
        // Lock the mutex due to the LVGL APIs are not thread-safe
        if (example_lvgl_lock(-1)) {
            task_delay_ms = lv_timer_handler();
            // Release the mutex
            example_lvgl_unlock();
        }
        if (task_delay_ms > EXAMPLE_LVGL_TASK_MAX_DELAY_MS) {
            task_delay_ms = EXAMPLE_LVGL_TASK_MAX_DELAY_MS;
        } else if (task_delay_ms < EXAMPLE_LVGL_TASK_MIN_DELAY_MS) {
            task_delay_ms = EXAMPLE_LVGL_TASK_MIN_DELAY_MS;
        }
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
}
/****************************************************************************************************************************** */
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    //lv_obj_t * img_wifi=lv_img_create(base_1_2);
    //lv_obj_set_size(img_wifi,16,16);
    //lv_obj_set_pos(img_wifi,300,3);
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        //lv_img_set_src(img_wifi,&wifi_off);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "Retry connecting to AP...");
        //lv_img_set_src(img_wifi,&wifi_off);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        //lv_img_set_src(img_wifi,&wifi_on);
        xTaskCreate(&http_test_task, "http_test_task", 8192, NULL, 5, NULL);
    }
}


void wifi_init_sta(void) {
    // 初始化 NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 初始化网络接口和事件循环
    ESP_ERROR_CHECK(esp_netif_init());//初始化tcpip协议栈
    ESP_ERROR_CHECK(esp_event_loop_create_default()); //创建一个默认系统事件调度循环，之后可以注册回调函数来处理系统的一些事件
    esp_netif_create_default_wifi_sta();//使用默认配置创建STA对象
    

    // 初始化 Wi-Fi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 注册事件处理函数
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    // 配置 Wi-Fi
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "Xiaomi 13",
            .password = "cyy123123",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,      // 加密方式
            
            .pmf_cfg = 
            {
                .capable = true,        // 启用保护管理帧
                .required = false       // 禁止仅与保护管理帧设备通信
            }
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA ,&wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi initialized, connecting to AP...");
}
/****************************************************************************************************************************** */

extern lv_obj_t * label_time;
extern lv_obj_t * label_date;

void vTask_lvgl_app(void *pvParameters) {
    //char strftime_buf[64];
    while (1) {
        struct tm timeinfo=esp_wait_sntp_sync();
        //strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        //ESP_LOGI(TAG, "The current date/time in Shanghai is: %s", strftime_buf);
        if(timeinfo.tm_min>=10&&timeinfo.tm_hour>=10)
        lv_label_set_text_fmt(label_time, "%d : %d",timeinfo.tm_hour ,timeinfo.tm_min);//动态显示
        else if(timeinfo.tm_min<10&&timeinfo.tm_hour>=10)
        lv_label_set_text_fmt(label_time, "%d : 0%d",timeinfo.tm_hour ,timeinfo.tm_min);//动态显示
        else if(timeinfo.tm_min>=10&&timeinfo.tm_hour<10)
        lv_label_set_text_fmt(label_time, "0%d : %d",timeinfo.tm_hour ,timeinfo.tm_min);//动态显示
        else if(timeinfo.tm_min<=10&&timeinfo.tm_hour<10)
        lv_label_set_text_fmt(label_time, "0%d : 0%d",timeinfo.tm_hour ,timeinfo.tm_min);//动态显示
        
        lv_label_set_text_fmt(label_date, "日期 : %d 年 %d 月 %d 日",timeinfo.tm_year+1900,timeinfo.tm_mon+1,timeinfo.tm_mday);//动态显示
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

}




void app_main(void)
{
    static lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)
    static lv_disp_drv_t disp_drv;      // contains callback functions

    base_1_flag=1;
    slider_flag=0;
    base_3_flag=0;
    base_4_flag=0;
    

    light_flag=0;
    kongtiao_flag=0;
    touying_flag=0;


    ESP_LOGI(TAG, "Turn off LCD backlight");
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_BK_LIGHT
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    ESP_LOGI(TAG, "Initialize SPI bus");
    spi_bus_config_t buscfg = {
        .sclk_io_num = EXAMPLE_PIN_NUM_SCLK,
        .mosi_io_num = EXAMPLE_PIN_NUM_MOSI,
        .miso_io_num = EXAMPLE_PIN_NUM_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = EXAMPLE_LCD_H_RES * 80 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = EXAMPLE_PIN_NUM_TOUCH_SDA,
        .scl_io_num = EXAMPLE_PIN_NUM_TOUCH_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000, // I2C时钟频率
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = EXAMPLE_PIN_NUM_LCD_DC,
        .cs_gpio_num = EXAMPLE_PIN_NUM_LCD_CS,
        .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,
        .lcd_param_bits = EXAMPLE_LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = example_notify_lvgl_flush_ready,
        .user_ctx = &disp_drv,
    };
    // Attach the LCD to the SPI bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
#if CONFIG_EXAMPLE_LCD_CONTROLLER_ILI9341
    ESP_LOGI(TAG, "Install st7796 panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7796(io_handle, &panel_config, &panel_handle));
#elif CONFIG_EXAMPLE_LCD_CONTROLLER_GC9A01
    ESP_LOGI(TAG, "Install GC9A01 panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(io_handle, &panel_config, &panel_handle));
#endif

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
#if CONFIG_EXAMPLE_LCD_CONTROLLER_GC9A01
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
#endif
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));

    // user can flush pre-defined pattern to the screen before we turn on the screen or backlight
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

#if CONFIG_EXAMPLE_LCD_TOUCH_ENABLED

    // I2C配置
     // I2C总线配置


    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_NUM_0, &tp_io_config, &tp_io_handle));


#if CONFIG_EXAMPLE_LCD_TOUCH_CONTROLLER_STMPE610
    ESP_LOGI(TAG, "Initialize touch controller FT6336");
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = EXAMPLE_LCD_H_RES,
        .y_max = EXAMPLE_LCD_V_RES,
        .rst_gpio_num = TOUCH_FT6336_RST, // 复位引脚
        .int_gpio_num = -1, // 中断引脚
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_ft5x06(tp_io_handle, &tp_cfg, &tp));
#endif // CONFIG_EXAMPLE_LCD_TOUCH_CONTROLLER_STMPE610
#endif // CONFIG_EXAMPLE_LCD_TOUCH_ENABLED

    //ESP_LOGI(TAG, "Turn on LCD backlight");
    gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL);

    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();
    // alloc draw buffers used by LVGL
    // it's recommended to choose the size of the draw buffer(s) to be at least 1/10 screen sized
    lv_color_t *buf1 = heap_caps_malloc(EXAMPLE_LCD_H_RES * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1);
    lv_color_t *buf2 = heap_caps_malloc(EXAMPLE_LCD_H_RES * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2);
    // initialize LVGL draw buffers
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, EXAMPLE_LCD_H_RES * 20);

    ESP_LOGI(TAG, "Register display driver to LVGL");
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = EXAMPLE_LCD_H_RES;
    disp_drv.ver_res = EXAMPLE_LCD_V_RES;
    disp_drv.rotated=LV_DISP_ROT_NONE;
    disp_drv.flush_cb = example_lvgl_flush_cb;
    disp_drv.drv_update_cb = example_lvgl_port_update_callback;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = panel_handle;
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

    ESP_LOGI(TAG, "Install LVGL tick timer");
    // Tick interface for LVGL (using esp_timer to generate 2ms periodic event)
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));

#if CONFIG_EXAMPLE_LCD_TOUCH_ENABLED
    static lv_indev_drv_t indev_drv;    // Input device driver (Touch)
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.disp = disp;
    indev_drv.read_cb = example_lvgl_touch_cb;
    indev_drv.user_data = tp;

    lv_indev_drv_register(&indev_drv);
#endif

    lvgl_mux = xSemaphoreCreateRecursiveMutex();
    assert(lvgl_mux);
    ESP_LOGI(TAG, "Create LVGL task");
    xTaskCreate(example_lvgl_port_task, "LVGL", EXAMPLE_LVGL_TASK_STACK_SIZE, NULL, EXAMPLE_LVGL_TASK_PRIORITY, NULL);
    
    ESP_LOGI(TAG, "Display LVGL Meter Widget");
    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (example_lvgl_lock(-1)) {
 //        example_lvgl_demo_ui(disp);
 //       lv_demo_music();
        lv_disp_set_rotation(disp, 1);
        USART_init();
        wifi_init_sta();
        gpio_set_level(GPIO_NUM_48, 1);
     /*   while(1){
            esp_wait_sntp_sync();
            vTaskDelay(100);
        }*/
        app_text();
        
        
/******************task_create********************** */
    TaskHandle_t xTaskHandle_lvgl_app = NULL;
    BaseType_t xReturn = xTaskCreate(
        vTask_lvgl_app,        // 任务函数
        "lvgl_app",       // 任务名称
        10000,                // 任务栈大小（以字为单位）
        NULL,                // 传递给任务函数的参数
        2,                   // 任务优先级
        &xTaskHandle_lvgl_app         // 任务句柄
    );

    
/************************************************** */
        example_lvgl_unlock();
    }
}
