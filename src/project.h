#ifndef _PROJECT_H_
#define _PROJECT_H_

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "mqtt.h"
#include "wifi_service.h"
#include "display.h"

enum {
	WIFI_DISCONNECTED,
	WIFI_CONNECTING,
	WIFI_CONNECTED,
	MQTT_DISCONNECTED,
	MQTT_CONNECTED
};

const char topic_pub[] = "z/event/temp";
const char topic_sub[] = "z/event/cmd";

struct sensor {
	uint32_t motor_rpm;
	float temp;
};

#endif