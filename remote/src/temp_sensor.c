#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

#include "temp_sensor.h"

const struct device *const dev = DEVICE_DT_GET_ONE(sensirion_sht3xd);

void temp_init(void)
{
	if (!device_is_ready(dev)) {
		printf("Device %s is not ready\n", dev->name);
	}
}

float temp_read(void)
{
	int rc;
	struct sensor_value temp, hum;

	rc = sensor_sample_fetch(dev);
	if (rc == 0) {
		rc = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
	}

	if (rc == 0) {
		rc = sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &hum);
	}

	if (rc != 0) {
		printf("SHT3XD: failed: %d\n", rc);
	}

	return sensor_value_to_double(&temp);
}