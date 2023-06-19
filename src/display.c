#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/display.h>
#include <lvgl.h>

#include "display.h"

static lv_obj_t *meter = NULL;
static lv_meter_indicator_t *indic = NULL;

static lv_obj_t *temp_label = NULL;
static lv_obj_t *wifi_symbol = NULL;
static lv_obj_t *title_label = NULL;
static lv_obj_t *motor_label = NULL;

void lv_motor_gauge_value(int32_t v)
{
	if (meter == NULL || indic == NULL) {
		return;
	}

	lv_meter_set_indicator_value(meter, indic, v);
}

void lv_temp_set_value(float temp)
{
	if (temp_label == NULL) {
		return;
	}

	char val[8];
	sprintf(val, "%0.1f°C", temp);
	lv_label_set_text(temp_label, val);
}

void lv_motor_gauge_display(void)
{
	meter = lv_meter_create(lv_scr_act());
	lv_obj_center(meter);
	lv_obj_set_size(meter, 180, 180);

	/*Add a scale first*/
	lv_meter_scale_t *scale = lv_meter_add_scale(meter);
	lv_meter_set_scale_range(meter, scale, 0, 240, 270, 120);
	lv_meter_set_scale_ticks(meter, scale, 36 + 1, 2, 10, lv_palette_main(LV_PALETTE_GREY));
	lv_meter_set_scale_major_ticks(meter, scale, 6, 4, 15, lv_color_black(), 10);

	/*Add a blue arc to the start*/
	indic = lv_meter_add_arc(meter, scale, 3, lv_palette_main(LV_PALETTE_BLUE), 0);
	lv_meter_set_indicator_start_value(meter, indic, 0);
	lv_meter_set_indicator_end_value(meter, indic, 40);

	/*Make the tick lines blue at the start of the scale*/
	indic = lv_meter_add_scale_lines(meter, scale, lv_palette_main(LV_PALETTE_BLUE),
					 lv_palette_main(LV_PALETTE_BLUE), false, 0);
	lv_meter_set_indicator_start_value(meter, indic, 0);
	lv_meter_set_indicator_end_value(meter, indic, 40);

	/*Add a red arc to the end*/
	indic = lv_meter_add_arc(meter, scale, 3, lv_palette_main(LV_PALETTE_RED), 0);
	lv_meter_set_indicator_start_value(meter, indic, 200);
	lv_meter_set_indicator_end_value(meter, indic, 240);

	/*Make the tick lines red at the end of the scale*/
	indic = lv_meter_add_scale_lines(meter, scale, lv_palette_main(LV_PALETTE_RED),
					 lv_palette_main(LV_PALETTE_RED), false, 0);
	lv_meter_set_indicator_start_value(meter, indic, 200);
	lv_meter_set_indicator_end_value(meter, indic, 240);

	/*Add a needle line indicator*/
	indic = lv_meter_add_needle_line(meter, scale, 4, lv_palette_main(LV_PALETTE_GREY), -10);
}

static void display_task(void)
{
	const struct device *display_dev;
	lv_obj_t *hello_world_label;
	lv_obj_t *count_label;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		printk("Device not ready, aborting test");
		return;
	}

	LV_IMG_DECLARE(zephyr_summit);
	// LV_IMG_DECLARE(esp_logo);
	// LV_IMG_DECLARE(esp_text);
	lv_obj_t *img_zephyr;
	// lv_obj_t *img_esp;
	// lv_obj_t *img_esp_text;

	// img_esp = lv_img_create(lv_scr_act());
	// lv_img_set_src(img_esp, &esp_logo);
	// lv_obj_align(img_esp, LV_ALIGN_CENTER, 0, -30);

	// img_esp_text = lv_img_create(lv_scr_act());
	// lv_img_set_src(img_esp_text, &esp_text);
	// lv_obj_align(img_esp_text, LV_ALIGN_BOTTOM_MID, 0, -40);

	// k_msleep(2000);
	display_blanking_off(display_dev);

	lv_obj_clean(lv_scr_act());
	img_zephyr = lv_img_create(lv_scr_act());
	lv_img_set_src(img_zephyr, &zephyr_summit);
	lv_obj_align(img_zephyr, LV_ALIGN_TOP_LEFT, 0, 0);

	lv_task_handler();

	k_msleep(5000);
	lv_obj_clean(lv_scr_act());

	temp_label = lv_label_create(lv_scr_act());
	lv_obj_align(temp_label, LV_ALIGN_TOP_RIGHT, -30, 5);

	wifi_symbol = lv_label_create(lv_scr_act());
	lv_obj_align(wifi_symbol, LV_ALIGN_TOP_RIGHT, -5, 5);
	lv_label_set_text(wifi_symbol, LV_SYMBOL_WIFI);

	title_label = lv_label_create(lv_scr_act());
	lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 100, 5);
	// char val[30] = "Zephyr Summit Demo";
	char val[30] = "ZEPHYR SUMMIT";
	lv_label_set_text(title_label, val);

	motor_label = lv_label_create(lv_scr_act());
	lv_obj_align(motor_label, LV_ALIGN_BOTTOM_MID, 0, -10);
	// char val[30] = "Zephyr Summit Demo";
	char rpm[30] = "RPM";
	lv_label_set_text(motor_label, rpm);

	/*Create an array for the points of the line*/
    static lv_point_t line_points[] = { {0, 25}, {318, 25}};

    /*Create a line and apply the new style*/
    lv_obj_t *line = lv_line_create(lv_scr_act());
    lv_line_set_points(line, line_points, 2);     /*Set the points*/

	lv_task_handler();
	lv_motor_gauge_display();

	for (;;) {
		lv_task_handler();
		k_sleep(K_MSEC(100));
	}
}

K_THREAD_DEFINE(display_thread, 3072, display_task, NULL, NULL, NULL, 5, 0, 0);
