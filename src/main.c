#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/data/json.h>
#include <zephyr/kernel.h>
#include <zephyr/ipc/rpmsg_service.h>

#include "project.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cpu0, CONFIG_LOG_DEFAULT_LEVEL);

/* sensor data */
static struct sensor evt_status;

/* RPMsg */
static int ep_id;
struct rpmsg_endpoint my_ept;
struct rpmsg_endpoint *ep = &my_ept;

static uint8_t m_state = WIFI_DISCONNECTED;

static int rpmsg_send_start(void)
{
	uint8_t msg = 0;
	return rpmsg_service_send(ep_id, &msg, 1);
}

void mqtt_response(const char *topic, uint32_t topic_len, const char *msg, uint32_t msg_len)
{
	LOG_INF("topic: %s", topic);
	LOG_INF("msg: %s", msg);
}

int main(void)
{
	char msg[50];

	/* wait endpoint to sync */
	while (!rpmsg_service_endpoint_is_bound(ep_id)) {
		k_sleep(K_MSEC(100));
	}

	/* send initial start command to CPU1 */
	rpmsg_send_start();

	/* update display values */
	lv_motor_gauge_value(evt_status.motor_rpm);
	lv_temp_set_value(evt_status.temp);

	/* init wifi and mqtt */
	wifi_init();
	mqtt_init(mqtt_response);

	for (;;) {

		switch (m_state) {
		case WIFI_DISCONNECTED:
			wifi_connect();
			m_state = WIFI_CONNECTING;
			break;

		case WIFI_CONNECTING:
			if (wifi_connected()) {
				m_state = WIFI_CONNECTED;
			} else {
				m_state = WIFI_DISCONNECTED;
			}
			break;

		case WIFI_CONNECTED:
			if (mqtt_connect_broker() == 0) {
				k_msleep(1000);
				mqtt_subscribe_to(topic_sub, strlen(topic_sub), 0);
				m_state = MQTT_CONNECTED;
			} else {
				m_state = WIFI_CONNECTING;
			}
			break;

		case MQTT_CONNECTED:
			if (!wifi_connected()) {
				mqtt_disconnect_broker();
				m_state = WIFI_DISCONNECTED;
			}

			if (mqtt_connected()) {
				sprintf(msg, "{\"temp\":%0.1f, \"rpm\":%d}", evt_status.temp,
					evt_status.motor_rpm);
				mqtt_publish_to(topic_pub, strlen(topic_pub), msg, strlen(msg), 0);
			} else {
				mqtt_disconnect_broker();
				m_state = WIFI_CONNECTING;
			}
			break;

		default:
			break;
		}

		k_msleep(1000);
	}

	return 0;
}

int endpoint_cb(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src, void *priv)
{
	memcpy(&evt_status, data, sizeof(evt_status));
	lv_motor_gauge_value(evt_status.motor_rpm);
	lv_temp_set_value(evt_status.temp);

	return RPMSG_SUCCESS;
}

/* Make sure we register endpoint before RPMsg Service is initialized. */
int register_endpoint(void)
{
	int status;

	status = rpmsg_service_register_endpoint("demo", endpoint_cb);

	if (status < 0) {
		printk("rpmsg_create_ept failed %d\n", status);
		return status;
	}

	ep_id = status;

	return 0;
}

SYS_INIT(register_endpoint, POST_KERNEL, CONFIG_RPMSG_SERVICE_EP_REG_PRIORITY);
