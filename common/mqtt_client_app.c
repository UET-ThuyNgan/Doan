#include <stdio.h>
#include <string.h>
#include "mqtt_client.h"
#include "esp_log.h"
#include "mqtt_client_app.h"

static const char *TAG = "MQTT_CLIENT_APP";
static esp_mqtt_client_handle_t client = NULL;

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event) {
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            esp_mqtt_client_subscribe(client, "v1/devices/me/rpc/request/+", 1);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            char data[64];
            memcpy(data, event->data, event->data_len);
            data[event->data_len] = '\0';
            printf("data = %s \n", data);
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;

        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

void mqtt_app_start(void) {
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = "mqtt://app.coreiot.io:1883",
        .username = "vJ563htLOcTiSvyfKQNS",
        .event_handle = mqtt_event_handler,
    };

    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
}


void mqtt_app_publish(const char *topic, const char *data, size_t len) {
    if (client && topic && data && len > 0) {
        esp_mqtt_client_publish(client, topic, data, len, 1, 0);
    } else {
        ESP_LOGW(TAG, "MQTT not connected or invalid params, skipping publish.");
    }
}

void mqtt_app_subscribe(const char *topic) {
    if (client && topic) {
        esp_mqtt_client_subscribe(client, topic, 1);
    }
}
