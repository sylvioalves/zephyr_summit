#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/data/json.h>
#include <zephyr/kernel.h>
#include <zephyr/ipc/rpmsg_service.h>

#include <stdio.h>
#include <string.h>

#include "buttons.h"
#include "temp_sensor.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cpu1, CONFIG_LOG_DEFAULT_LEVEL);

static struct k_work motor_work;

#include <zephyr/drivers/pwm.h>
#define MIN_PERIOD PWM_SEC(1U) / 128U
#define MAX_PERIOD PWM_SEC(1U)

static uint32_t motor_period = 6400;
static bool motor_running = false;

static const struct pwm_dt_spec pwm_motor = PWM_DT_SPEC_GET(DT_ALIAS(pwm_motor));

static int ep_id;

struct sensor {
	uint32_t motor_rpm;
	float temp;
};

static struct sensor evt_status = {.motor_rpm = 0};

struct motor_data {
	uint32_t speed;
};

static int send_data_to_cpu0(void)
{
	int sent = rpmsg_service_send(ep_id, &evt_status, sizeof(struct sensor));
	return sent;
}

static void motor_event(struct k_work *work)
{
	if (motor_running) {
		if (motor_period > 0) {
			pwm_set_dt(&pwm_motor, PWM_HZ(motor_period), PWM_HZ(motor_period) / 2);
		} else {
			pwm_set_pulse_dt(&pwm_motor, 0);
		}
	} else {
		/* Disable the PWM */
		pwm_set_pulse_dt(&pwm_motor, 0);
	}

	send_data_to_cpu0();
}

static void motor_start(void)
{
	if (!device_is_ready(pwm_motor.dev)) {
		printk("Error: PWM device %s is not ready\n", pwm_motor.dev->name);
		return;
	}

	k_work_init(&motor_work, motor_event);
	k_work_submit(&motor_work);
}

static void button_cb(int button)
{
	if (button == BTN_LEFT) {
		if (motor_period >= 160) {
			motor_period -= 160;
		}
	}

	if (button == BTN_RIGHT) {
		if (motor_period <= 6400 - 160) {
			motor_period += 160;
		}
	}

	if (button == BTN_CENTER) {
		if (!motor_running) {
			motor_running = true;
		} else {
			motor_running = false;
		}
	}

	if (motor_running) {
		evt_status.motor_rpm = motor_period * 60 / 1600;
	} else {
		evt_status.motor_rpm = 0;
	}

	k_work_submit(&motor_work);
}

int main(void)
{
	char msg[50];
	float temp;

	temp_init();
	motor_start();
	buttons_init(button_cb);

	for (;;) {
		evt_status.temp = temp_read();
		LOG_INF("temp sensor: %0.1fºC", evt_status.temp);
		send_data_to_cpu0();

		k_msleep(2000);
	}

	return 0;
}

int endpoint_cb(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src, void *priv)
{
	return RPMSG_SUCCESS;
}

/* Make sure we register endpoint before RPMsg Service is initialized. */
int register_endpoint(void)
{
	int status;

	status = rpmsg_service_register_endpoint("demo", endpoint_cb);

	if (status < 0) {
		ets_printf("rpmsg_create_ept failed %d\n", status);
		return status;
	}

	ep_id = status;

	return 0;
}

SYS_INIT(register_endpoint, POST_KERNEL, CONFIG_RPMSG_SERVICE_EP_REG_PRIORITY);
