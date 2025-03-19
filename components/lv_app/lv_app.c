#include <stdio.h>
#include "lv_app.h"
#include "lwip/apps/sntp.h"
#include "time.h"


lv_obj_t * base_1;//创建背景板(主页)

lv_obj_t * slider;//（设置页）

extern struct tm timeinfo;

bool count_img_1=true;
static void imgbtn1_cb(lv_event_t * e)//设置按钮回调函数
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * img1 = lv_event_get_target(e);
    lv_obj_t * base_1_1=lv_obj_get_parent(img1);
    
    if(code == LV_EVENT_CLICKED) {
        if(count_img_1==false) count_img_1=true;
        else count_img_1=false;
        if(count_img_1==false)
            lv_img_set_src(img1, &light_off);
        else
            lv_img_set_src(img1, &light_on);
    }
//    else if(code==LV_EVENT_LONG_PRESSED)
//    {
//        lv_obj_add_flag(base_1,LV_OBJ_FLAG_HIDDEN);
//    }
}

bool count_img_2=true;
static void imgbtn2_cb(lv_event_t * e)//设置按钮回调函数
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * img2 = lv_event_get_target(e);
    lv_obj_t * base_1_1=lv_obj_get_parent(img2);
    
    if(code == LV_EVENT_CLICKED) {
        //lv_obj_add_flag(base_1,LV_OBJ_FLAG_HIDDEN);
        if(count_img_2==false) count_img_2=true;
        else count_img_2=false;
        if(count_img_2==false)
            lv_img_set_src(img2, &kt_off);
        else
            lv_img_set_src(img2, &kt_on);
    }
}

bool count_img_3=true;
static void imgbtn3_cb(lv_event_t * e)//设置按钮回调函数
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * img3 = lv_event_get_target(e);
    lv_obj_t * base_1_1=lv_obj_get_parent(img3);
    
    if(code == LV_EVENT_CLICKED) {
        //lv_obj_add_flag(base_1,LV_OBJ_FLAG_HIDDEN);
        if(count_img_3==false) count_img_3=true;
        else count_img_3=false;
        if(count_img_3==false)
            lv_img_set_src(img3, &bg_off);
        else
            lv_img_set_src(img3, &bg_on);
    }
}

static void back_btn_cb(lv_event_t * e)//设置按钮回调函数
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    //lv_obj_t * base_1_1=lv_obj_get_parent(img3);
    
    if(code == LV_EVENT_CLICKED) {
        lv_obj_del(slider);
        lv_obj_clear_flag(base_1,LV_OBJ_FLAG_HIDDEN);
        //lv_obj_add_flag(slider,LV_OBJ_FLAG_HIDDEN);
        lv_obj_del(btn);
    }
}

//extern const lv_img_dsc_t bg;
void setting_act(void)
{
     slider = lv_obj_create(lv_scr_act());
    lv_obj_set_size(slider, 480, 320); // 设置容器大小
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 0); // 将容器居中
    lv_obj_set_flex_flow(slider, LV_FLEX_FLOW_ROW); // 设置水平布局
    lv_obj_set_style_pad_all(slider, 50, LV_PART_MAIN); // 设置内边距
    lv_obj_set_style_pad_gap(slider, 100, LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0xffffff), LV_PART_MAIN); // 设置背景颜色
    lv_obj_set_style_border_width(slider, 0, LV_PART_MAIN); // 去掉边框
    lv_obj_set_scroll_snap_x(slider, LV_SCROLL_SNAP_CENTER); // 设置水平滑动对齐到中心
    lv_obj_set_scroll_dir(slider, LV_DIR_HOR); // 允许水平滑动

    // 添加图片（不需要手动设置位置）
    lv_obj_t * img1 = lv_imgbtn_create(slider);
    lv_img_set_src(img1, &light_off);
    lv_obj_set_size(img1,200,200);
    lv_obj_add_event_cb(img1, imgbtn1_cb, LV_EVENT_ALL, NULL);

    lv_obj_t * img2 = lv_imgbtn_create(slider);
    lv_img_set_src(img2, &kt_off);
    lv_obj_set_size(img2,200,200);
    lv_obj_add_event_cb(img2, imgbtn2_cb, LV_EVENT_ALL, NULL);

    lv_obj_t * img3 = lv_imgbtn_create(slider);
    lv_img_set_src(img3, &bg_off);
    lv_obj_set_size(img3,200,200);
    lv_obj_add_event_cb(img3, imgbtn3_cb, LV_EVENT_ALL, NULL);

    lv_obj_t * back_btn=lv_btn_create(lv_scr_act());
    lv_obj_set_size(back_btn,40,30);
    lv_obj_align_to(back_btn, slider, LV_ALIGN_BOTTOM_RIGHT, 20, 30);
    lv_obj_add_event_cb(back_btn, back_btn_cb, LV_EVENT_ALL, NULL);

}

static void imgbtn_event_cb_1(lv_event_t * e)//设置按钮回调函数
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * imgbtn = lv_event_get_target(e);
//    lv_obj_t * base_1=lv_obj_get_parent(imgbtn);

    if(code == LV_EVENT_CLICKED) {
        // 处理按钮点击事件
        LV_LOG_USER("Image button clicked!");
        lv_obj_add_flag(base_1,LV_OBJ_FLAG_HIDDEN);
        setting_act();
    }
}

static void set_img_x(void * img, int32_t x) {//动画回调函数
    lv_obj_set_x(img, x);
}

lv_anim_t anim_people;
static void imgbtn_event_cb_people(lv_event_t * e)//头像按钮回调函数
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * imgbtn = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED) {
        lv_anim_start(&anim_people);                        // 启动动画
    }
}
lv_obj_t * base_3;

static void set_label_opa(void * label, int32_t opa) {//信息渐显动画回调函数
    lv_obj_set_style_opa(label, opa, 0);  // 设置 Label 的透明度
}

static void anim_ready_cb(lv_anim_t * anim)//动画结束回调函数
{
    lv_obj_t * img = anim->var;
    lv_obj_t * label_people;
    label_people = lv_label_create(base_3); //日期中文字体
    lv_label_set_text(label_people, "name:Join\nphone:158xxxxxxxx\nemail:xxxxxxxxxx@qq.com\n");
    static lv_style_t label_style_2; 
    lv_style_init(&label_style_2);
    lv_style_set_text_font(&label_style_2,&lv_font_montserrat_24);
    lv_obj_set_pos(label_people,240,100);
    lv_obj_set_style_text_color(label_people, lv_color_hex(0x000000), LV_PART_MAIN); 
    lv_obj_add_style(label_people, &label_style_2, 0);

    lv_anim_t anim_label_people;//信息渐显动画
    lv_anim_init(&anim_label_people);
    lv_anim_set_var(&anim_label_people, label_people);
    lv_anim_set_exec_cb(&anim_label_people, set_label_opa);
    lv_anim_set_values(&anim_label_people,LV_OPA_TRANSP , LV_OPA_COVER);
    lv_anim_set_time(&anim_label_people, 300);
    lv_anim_set_repeat_count(&anim_label_people, 0);      // 一次播放
    lv_anim_start(&anim_label_people);
}

static void back_btn_cb_3(lv_event_t * e)//设置按钮回调函数
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    //lv_obj_t * base_1_1=lv_obj_get_parent(img3);
    
    if(code == LV_EVENT_CLICKED) {
        lv_obj_del(base_3);
        lv_obj_clear_flag(base_1,LV_OBJ_FLAG_HIDDEN);
        //lv_obj_add_flag(slider,LV_OBJ_FLAG_HIDDEN);
        lv_obj_del(btn);
    }
}

void phone_act(void)
{
    static lv_style_t style_base_1; //为背景板创建一个样式
    lv_style_init(&style_base_1); 

    lv_style_set_bg_color(&style_base_1, lv_color_hex(0xFFFFFF)); //背景板为黑色

    base_3=lv_obj_create(lv_scr_act());
    lv_obj_set_size(base_3,480,320);
    lv_obj_add_style(base_3, &style_base_1, LV_STATE_DEFAULT); 


    lv_obj_t * image_people=lv_imgbtn_create(base_3);
    lv_obj_align(image_people,LV_ALIGN_CENTER,0,0);
    lv_obj_set_size(image_people,200,200);
    lv_imgbtn_set_src(image_people,LV_IMGBTN_STATE_RELEASED,NULL,&people,NULL);
    lv_obj_add_event_cb(image_people, imgbtn_event_cb_people, LV_EVENT_ALL, NULL);

   
    lv_anim_init(&anim_people);                          // 初始化动画
    lv_anim_set_var(&anim_people, image_people);                  // 设置动画目标对象
    lv_anim_set_exec_cb(&anim_people, set_img_x);        // 设置动画回调函数
    lv_anim_set_values(&anim_people, lv_obj_get_x(image_people),-100);  // 起始值和结束值
    lv_anim_set_time(&anim_people, 1000);               // 动画持续时间（2 秒）
    lv_anim_set_repeat_count(&anim_people, 0);  // 单次播放
    lv_anim_set_ready_cb(&anim_people, anim_ready_cb);
    
    lv_obj_t * back_btn=lv_btn_create(lv_scr_act());
    lv_obj_set_size(back_btn,40,30);
    lv_obj_align_to(back_btn, base_3, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_add_event_cb(back_btn, back_btn_cb_3, LV_EVENT_ALL, NULL);
}

static void imgbtn_event_cb_2(lv_event_t * e)//电话按钮回调函数
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * imgbtn = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED) {
        // 处理按钮点击事件
        LV_LOG_USER("Image button clicked!");
        lv_obj_add_flag(base_1,LV_OBJ_FLAG_HIDDEN);
        phone_act();
    }
}

static void imgbtn_event_cb_3(lv_event_t * e)//课表按钮回调函数
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * imgbtn = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED) {
        // 处理按钮点击事件
        LV_LOG_USER("Image button clicked!");
        lv_obj_add_flag(imgbtn,LV_OBJ_FLAG_HIDDEN);
    }
}
lv_obj_t * label_time;
lv_obj_t * label_date;
lv_obj_t * base_1_2;
void app_text(void)
{
    LV_IMG_DECLARE(error);
    LV_IMG_DECLARE(setting);
    LV_IMG_DECLARE(call);
    LV_IMG_DECLARE(timing);
    LV_IMG_DECLARE(light_on);
    LV_IMG_DECLARE(light_off);
    LV_IMG_DECLARE(kt_on);
    LV_IMG_DECLARE(kt_off);
    LV_IMG_DECLARE(bg_on);
    LV_IMG_DECLARE(bg_off);
    LV_IMG_DECLARE(wifi_on);
    LV_IMG_DECLARE(wifi_off);
    LV_IMG_DECLARE(people);
    //LV_FONT_DECLARE(LV_FONT_MONTSERRAT_30);

    static lv_style_t style_base_1; //为背景板创建一个样式
    lv_style_init(&style_base_1); 

    lv_style_set_bg_color(&style_base_1, lv_color_hex(0xFFFFFF)); //背景板为黑色

    base_1=lv_obj_create(lv_scr_act());
    lv_obj_set_size(base_1,480,320);
    lv_obj_add_style(base_1, &style_base_1, LV_STATE_DEFAULT); 


    label_time = lv_label_create(base_1); //大字时间标签
    lv_label_set_text(label_time, "Hello, LVGL!");
    char *a="00";
    char *b="00";
    lv_label_set_text_fmt(label_time, "%s : %s", a,b);//动态显示
    static lv_style_t label_style; 
    lv_style_init(&label_style);
    lv_style_set_text_font(&label_style,&lv_font_montserrat_48);//设置最大字体
    lv_obj_set_pos(label_time,20,40);
    lv_obj_set_style_text_color(label_time, lv_color_hex(0x000000), LV_PART_MAIN); //白色
    lv_obj_add_style(label_time, &label_style, 0);



    label_date = lv_label_create(label_time); //日期中文字体
    lv_label_set_text(label_date, "日期:");
    static lv_style_t label_style_2; 
    lv_style_init(&label_style_2);
    lv_style_set_text_font(&label_style_2,&lv_font_simsun_16_cjk);
    lv_obj_set_pos(label_date,0,60);
    lv_obj_set_style_text_color(label_date, lv_color_hex(0x000000), LV_PART_MAIN); 
    lv_obj_add_style(label_date, &label_style_2, 0);


    

    lv_obj_t * imgbtn_1 = lv_imgbtn_create(base_1);//创建设置按钮
    lv_obj_set_size(imgbtn_1, 60, 60);  // 设置按钮大小
    lv_imgbtn_set_src(imgbtn_1,LV_IMGBTN_STATE_RELEASED,NULL,&setting,NULL);
    lv_obj_align(imgbtn_1,LV_ALIGN_CENTER,180,-90);
    lv_obj_add_event_cb(imgbtn_1, imgbtn_event_cb_1, LV_EVENT_ALL, NULL);//设置回调函数

    lv_obj_t * imgbtn_2 = lv_imgbtn_create(base_1);//创建电话按钮
    lv_obj_set_size(imgbtn_2, 60, 60);  // 设置按钮大小
    lv_imgbtn_set_src(imgbtn_2,LV_IMGBTN_STATE_RELEASED,NULL,&call,NULL);
    lv_obj_align(imgbtn_2,LV_ALIGN_CENTER,180,0);
    lv_obj_add_event_cb(imgbtn_2, imgbtn_event_cb_2, LV_EVENT_ALL, NULL);

    lv_obj_t * imgbtn_3 = lv_imgbtn_create(base_1);//创建课表按钮
    lv_obj_set_size(imgbtn_3, 60, 60);  // 设置按钮大小
    lv_imgbtn_set_src(imgbtn_3,LV_IMGBTN_STATE_RELEASED,NULL,&timing,NULL);
    lv_obj_align(imgbtn_3,LV_ALIGN_CENTER,180,90);
    lv_obj_add_event_cb(imgbtn_3, imgbtn_event_cb_3, LV_EVENT_ALL, NULL);


    static lv_style_t style_base_1_1; //为左下角显示框创造容器
    lv_style_init(&style_base_1_1); 

    lv_style_set_bg_color(&style_base_1_1, lv_color_hex(0xffffff)); //黑色

    static lv_style_t style_shadow_1_1;
    lv_style_init(&style_shadow_1_1);
    lv_style_set_shadow_width(&style_shadow_1_1, 10);
    lv_style_set_shadow_spread(&style_shadow_1_1, 5);//边框
    lv_style_set_shadow_color(&style_shadow_1_1, lv_color_hex(0x333333));

    lv_obj_t * base_1_1=lv_obj_create(base_1);
    lv_obj_set_size(base_1_1,260,120);
    lv_obj_set_pos(base_1_1,20,160);
    lv_obj_add_style(base_1_1, &style_base_1_1, LV_STATE_DEFAULT); 
    lv_obj_set_style_bg_opa(base_1_1, LV_OPA_80, LV_PART_MAIN);//透明度80
    lv_obj_add_style(base_1_1, &style_shadow_1_1, 0);

    lv_obj_t * label_base_1_1 = lv_label_create(base_1_1); //设置当前课程详情显示标签
    char *teacher="MR.JON";
    char *course="MATH";
    char *class="NO.11";
    lv_label_set_text_fmt(label_base_1_1, "COURSE : %s\n\nCLASS : %s\n\nTEACHER : %s", course,class,teacher);
    lv_obj_set_style_text_color(label_base_1_1, lv_color_hex(0x000000), LV_PART_MAIN);
    
    lv_obj_t * line_1_1=lv_line_create(base_1);
    static lv_point_t line_points[] = {
        {0, 25},
        {480, 25}
    };
    lv_line_set_points(line_1_1, line_points, 2);
    static lv_style_t line_style;
    lv_style_init(&line_style);
    lv_style_set_line_color(&line_style, lv_color_hex(0xaaaaaa));
    lv_style_set_line_width(&line_style, 5);
    lv_obj_add_style(line_1_1, &line_style, 0);

    lv_obj_t * label_base_1_2 = lv_label_create(base_1); //创建天气显示标签
    char *weather="sunny";
    lv_label_set_text_fmt(label_base_1_2, "%s", weather);
    //lv_obj_set_size(label_base_1_2,210,0);
    lv_obj_align(label_base_1_2, LV_ALIGN_OUT_TOP_MID, 0, 0);
    lv_obj_set_style_text_color(label_base_1_2, lv_color_hex(0x0000ff), LV_PART_MAIN);
 /*************************************************************************************************/
    static lv_style_t style_bg;
    static lv_style_t style_indic;

   /* lv_style_init(&style_bg);//电量显示
    lv_style_set_border_color(&style_bg, lv_color_hex(0x000000));
    lv_style_set_border_width(&style_bg, 2);
    lv_style_set_pad_all(&style_bg, 6); 
    lv_style_set_radius(&style_bg, 6);*/
    //lv_style_set_anim_speed(&style_bg, 1000);
/*
    lv_style_init(&style_indic);
    lv_style_set_bg_opa(&style_indic, LV_OPA_COVER);
    lv_style_set_bg_color(&style_indic, lv_color_hex(0xff000));
    lv_style_set_radius(&style_indic, 3);

    lv_obj_t * bar = lv_bar_create(base_1);
    lv_obj_remove_style_all(bar);  
    lv_obj_add_style(bar, &style_bg, 0);
    lv_obj_add_style(bar, &style_indic, LV_PART_INDICATOR);

    lv_obj_set_size(bar, 30, 10);
    lv_obj_set_pos(bar,20,5);
    lv_bar_set_value(bar, 100, LV_ANIM_ON);*/
    /*************************************************************************************************/

}


