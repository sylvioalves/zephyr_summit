#include <zephyr/kernel.h>
#include <errno.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>

#include "mqtt_service.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wifi_sample, LOG_LEVEL_INF);

/* clang-format off */
#define WIFI_SAMPLE_MGMT_EVENTS (NET_EVENT_WIFI_SCAN_RESULT |		\
				NET_EVENT_WIFI_SCAN_DONE |		\
				NET_EVENT_WIFI_CONNECT_RESULT |		\
				NET_EVENT_WIFI_DISCONNECT_RESULT |  \
				NET_EVENT_WIFI_TWT)
/* clang-format on */

static struct net_mgmt_event_callback wifi_sample_mgmt_cb;

static struct {
	const struct shell *sh;
	union {
		struct {
			uint8_t connecting: 1;
			uint8_t disconnecting: 1;
			uint8_t _unused: 6;
		};
		uint8_t all;
	};
} context;

static uint32_t scan_result;

char *net_byte_to_hex_glob(char *ptr, uint8_t byte, char base, bool pad)
{
	int i, val;

	for (i = 0, val = (byte & 0xf0) >> 4; i < 2; i++, val = byte & 0x0f) {
		if (i == 0 && !pad && !val) {
			continue;
		}
		if (val < 10) {
			*ptr++ = (char)(val + '0');
		} else {
			*ptr++ = (char)(val - 10 + base);
		}
	}

	*ptr = '\0';

	return ptr;
}

char *net_sprint_ll_addr_buf_glob(const uint8_t *ll, uint8_t ll_len, char *buf, int buflen)
{
	uint8_t i, len, blen;
	char *ptr = buf;

	if (ll == NULL) {
		return "<unknown>";
	}

	switch (ll_len) {
	case 8:
		len = 8U;
		break;
	case 6:
		len = 6U;
		break;
	case 2:
		len = 2U;
		break;
	default:
		len = 6U;
		break;
	}

	for (i = 0U, blen = buflen; i < len && blen > 0; i++) {
		ptr = net_byte_to_hex_glob(ptr, (char)ll[i], 'A', true);
		*ptr++ = ':';
		blen -= 3U;
	}

	if (!(ptr - buf)) {
		return NULL;
	}

	*(ptr - 1) = '\0';
	return buf;
}

static void handle_wifi_scan_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_scan_result *entry = (const struct wifi_scan_result *)cb->info;
	uint8_t mac_string_buf[sizeof("xx:xx:xx:xx:xx:xx")];

	scan_result++;

	if (scan_result == 1U) {
		LOG_INF("%-4s | %-32s %-5s | %-13s | %-4s | %-15s | %s", "Num", "SSID", "(len)",
			"Chan (Band)", "RSSI", "Security", "BSSID");
	}

	LOG_INF("%-4d | %-32s %-5u | %-4u (%-6s) | %-4d | %-15s | %s", scan_result, entry->ssid,
		entry->ssid_length, entry->channel, wifi_band_txt(entry->band), entry->rssi,
		wifi_security_txt(entry->security),
		((entry->mac_length)
			 ? net_sprint_ll_addr_buf_glob(entry->mac, WIFI_MAC_ADDR_LEN,
						       mac_string_buf, sizeof(mac_string_buf))
			 : ""));
}

static void handle_wifi_scan_done(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status = (const struct wifi_status *)cb->info;

	if (status->status) {
		LOG_INF("Scan request failed (%d)", status->status);
	} else {
		LOG_INF("Scan request done");
	}

	scan_result = 0U;
}

static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status = (const struct wifi_status *)cb->info;

	if (status->status) {
		LOG_INF("Connection request failed (%d)", status->status);
	} else {
		LOG_INF("Connected");
	}

	context.connecting = false;
}

static void handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status = (const struct wifi_status *)cb->info;

	if (context.disconnecting) {
		LOG_INF("Disconnection request %s (%d)", status->status ? "failed" : "done",
			status->status);
		context.disconnecting = false;
	} else {
		LOG_INF("Disconnected");
	}
}

static void handle_wifi_twt_event(struct net_mgmt_event_callback *cb)
{
	const struct wifi_twt_params *resp = (const struct wifi_twt_params *)cb->info;

	if (resp->resp_status == WIFI_TWT_RESP_RECEIVED) {
		LOG_INF("TWT response: %s for dialog: %d and flow: %d",
			wifi_twt_setup_cmd2str[resp->setup_cmd], resp->dialog_token, resp->flow_id);

		/* If accepted, then no need to print TWT params */
		if (resp->setup_cmd != WIFI_TWT_SETUP_CMD_ACCEPT) {
			LOG_INF("TWT parameters: trigger: %s wake_interval: %d us, interval: %lld "
				"us",
				resp->setup.trigger ? "trigger" : "no_trigger",
				resp->setup.twt_wake_interval, resp->setup.twt_interval);
		}
	} else {
		LOG_INF("TWT response timed out");
	}
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
				    struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WIFI_SCAN_RESULT:
		handle_wifi_scan_result(cb);
		break;
	case NET_EVENT_WIFI_SCAN_DONE:
		handle_wifi_scan_done(cb);
		break;
	case NET_EVENT_WIFI_CONNECT_RESULT:
		handle_wifi_connect_result(cb);
		break;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		handle_wifi_disconnect_result(cb);
		break;
	case NET_EVENT_WIFI_TWT:
		handle_wifi_twt_event(cb);
		break;
	default:
		break;
	}
}

static void mqtt_service_user_cb(mqtt_service_event_t *evt)
{
	switch (evt->type) {
	case MQTT_SERVICE_EVT_CONNECTED:
		LOG_INF("MQTT Client connected!");
		break;

	case MQTT_SERVICE_EVT_DISCONNECTED:
		LOG_WRN("MQTT Client disconnected! Reason: %d", evt->data.disc_reason);
		break;

	case MQTT_SERVICE_EVT_BROKER_ACK:
		LOG_INF("MQTT Broker has acknowledged message, MID = %d", evt->data.ack.message_id);
		break;

	case MQTT_SERVICE_EVT_DATA_RECEIVED:
		LOG_INF("Received %d bytes from topic: %.*s", evt->data.publish.msg_length,
			evt->data.publish.topic_name_length, evt->data.publish.p_topic_name);
		LOG_HEXDUMP_INF(evt->data.publish.p_msg_data, evt->data.publish.msg_length,
				"Message");
		break;

	default:
		LOG_WRN("Uknown event from MQTT service module");
		break;
	}
}

void mqtt_init(void)
{
	net_mgmt_init_event_callback(&wifi_sample_mgmt_cb, wifi_mgmt_event_handler,
				     WIFI_SAMPLE_MGMT_EVENTS);

	net_mgmt_add_event_callback(&wifi_sample_mgmt_cb);

	int err = 0;

	err = mqtt_service_init(mqtt_service_user_cb);
	if (err) {
		LOG_ERR("Failed to initialize MQTT service");
	}
}