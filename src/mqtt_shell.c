#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>

#include "mqtt_service.h"

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL

LOG_MODULE_REGISTER(mqtt_shell);

static int cmd_mqtt_connect(const struct shell *sh, size_t argc, char **argv)
{
	int ret = mqtt_service_connect();
	if (ret) {
		shell_error(sh, "Failed to connect to mqtt server, err: %d", ret);
	}

	return 0;
}

static int cmd_mqtt_disconnect(const struct shell *sh, size_t argc, char **argv)
{
	int ret = mqtt_service_disconnect();
	if (ret) {
		shell_error(sh, "Failed to disconnect from mqtt server, err: %d", ret);
	}

	return 0;
}

static int cmd_mqtt_publish(const struct shell *sh, size_t argc, char **argv)
{
	uint8_t qos = 0;

	if (argc > 3) {
		qos = atoi(argv[3]);

		if (qos < 0 || qos > 2) {
			shell_error(sh, "QoS should be in range [0,2]");
			return -EINVAL;
		}
	}

	shell_info(sh, "Publishing %s to %s topic with QoS: %d", argv[2], argv[1], qos);

	mqtt_service_publish(argv[1], strlen(argv[1]), argv[2], strlen(argv[2]), qos);

	return 0;
}

static int cmd_mqtt_subscribe(const struct shell *sh, size_t argc, char **argv)
{
	uint8_t qos = 0;

	if (argc > 2) {
		qos = atoi(argv[2]);

		if (qos < 0 || qos > 2) {
			shell_error(sh, "QoS should be in range [0,2]");
			return -EINVAL;
		}
	}

	shell_info(sh, "Subscribing to %s topic with QoS: %d", argv[1], qos);

	mqtt_service_subscribe(argv[1], strlen(argv[1]), qos);

	return 0;
}

static int cmd_mqtt_unsubscribe(const struct shell *sh, size_t argc, char **argv)
{
	shell_info(sh, "Unsubscribing from %s topic.", argv[1]);

	mqtt_service_unsubscribe(argv[1], strlen(argv[1]));

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_mqtt_shell,

	SHELL_CMD_ARG(connect, NULL, "Connect MQTT", cmd_mqtt_connect, 1, 0),
	SHELL_CMD_ARG(disconnect, NULL, "Disconnect MQTT", cmd_mqtt_disconnect, 1, 0),
	SHELL_CMD_ARG(publish, NULL, "Publish to mqtt topic", cmd_mqtt_publish, 3, 1),
	SHELL_CMD_ARG(subscribe, NULL, "Subscribe to mqtt topic", cmd_mqtt_subscribe, 2, 1),
	SHELL_CMD_ARG(unsubscribe, NULL, "Unsubscribe to mqtt topic", cmd_mqtt_unsubscribe, 2, 0),

	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(mqtt, &sub_mqtt_shell, "mqtt commands", NULL);
