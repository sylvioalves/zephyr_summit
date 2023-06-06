/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

static uint32_t count;

#ifdef CONFIG_GPIO
static struct gpio_dt_spec button_gpio = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0});
static struct gpio_callback button_callback;

static void button_isr_callback(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	count = 0;
}
#endif

int main(void)
{
	char count_str[11] = {0};
	const struct device *display_dev;
	lv_obj_t *hello_world_label;
	lv_obj_t *count_label;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return 0;
	}

	LV_IMG_DECLARE(zephyr_summit);
	LV_IMG_DECLARE(esp_logo);
	LV_IMG_DECLARE(esp_text);
	lv_obj_t *img_zephyr;
	lv_obj_t *img_esp;
	lv_obj_t *img_esp_text;

	img_esp = lv_img_create(lv_scr_act());
	lv_img_set_src(img_esp, &esp_logo);
	lv_obj_align(img_esp, LV_ALIGN_CENTER, 0, -30);

	img_esp_text = lv_img_create(lv_scr_act());
	lv_img_set_src(img_esp_text, &esp_text);
	lv_obj_align(img_esp_text, LV_ALIGN_BOTTOM_MID, 0, -40);

	display_blanking_off(display_dev);
	lv_task_handler();

	k_msleep(3000);

	img_zephyr = lv_img_create(lv_scr_act());
	lv_img_set_src(img_zephyr, &zephyr_summit);
	lv_obj_align(img_zephyr, LV_ALIGN_TOP_LEFT, 0, 0);

	// for (;;) {
		lv_task_handler();
		k_sleep(K_MSEC(10));
	// }

#ifdef CONFIG_GPIO
	if (device_is_ready(button_gpio.port)) {
		int err;

		err = gpio_pin_configure_dt(&button_gpio, GPIO_INPUT);
		if (err) {
			LOG_ERR("failed to configure button gpio: %d", err);
			return 0;
		}

		gpio_init_callback(&button_callback, button_isr_callback, BIT(button_gpio.pin));

		err = gpio_add_callback(button_gpio.port, &button_callback);
		if (err) {
			LOG_ERR("failed to add button callback: %d", err);
			return 0;
		}

		err = gpio_pin_interrupt_configure_dt(&button_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		if (err) {
			LOG_ERR("failed to enable button callback: %d", err);
			return 0;
		}
	}
#endif

	mqtt_init();
}
