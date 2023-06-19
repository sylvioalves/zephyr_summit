/*
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "buttons.h"

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || !DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define DT_SPEC_AND_COMMA(node_id, prop, idx) ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)};

static struct adc_sequence sequence;

static void (*btn_action)(uint8_t button);

void buttons_init(void *callback)
{
	int err;

	if (callback != NULL) {
		btn_action = callback;
	}

	/* Configure channels individually prior to sampling. */
	if (!device_is_ready(adc_channels[0].dev)) {
		printk("ADC/Button controller device %s not ready\n", adc_channels[0].dev->name);
		return;
	}

	err = adc_channel_setup_dt(&adc_channels[0]);
	if (err < 0) {
		printk("Could not setup channel (%d)\n", err);
		return;
	}
}

static void button_task(void)
{
	int err;
	int32_t val_mv;
	uint32_t count = 0;
	uint16_t buf;
	struct adc_sequence sequence = {
		.buffer = &buf,
		.buffer_size = sizeof(buf),
		.oversampling = 2,
	};

	for (;;) {
		(void)adc_sequence_init_dt(&adc_channels[0], &sequence);

		err = adc_read(adc_channels[0].dev, &sequence);
		if (err < 0) {
			printk("Could not read (%d)\n", err);
			continue;
		}

		if (adc_channels[0].channel_cfg.differential) {
			val_mv = (int32_t)((int16_t)buf);
		} else {
			val_mv = (int32_t)buf;
		}
		err = adc_raw_to_millivolts_dt(&adc_channels[0], &val_mv);
		if (err < 0) {
			// printk(" (value in mV not available)\n");
		} else {
			if (btn_action != NULL) {
				// btn_not_pressed = 3079mV
				// btn_left = 2400mV
				// btn_center = 1987mV
				// btn_right = 807mV
				if (val_mv > 2200 && val_mv < 2600) {
					btn_action(BTN_LEFT);
					k_sleep(K_MSEC(200));
				} else if (val_mv > 1800 && val_mv < 2100) {
					btn_action(BTN_CENTER);
					k_sleep(K_MSEC(200));
				} else if (val_mv > 600 && val_mv < 1000) {
					btn_action(BTN_RIGHT);
					k_sleep(K_MSEC(200));
				}
			}
		}

		k_sleep(K_MSEC(10));
	}
}

K_THREAD_DEFINE(buttons_thread, 1024, button_task, NULL, NULL, NULL, 3, 0, 0);