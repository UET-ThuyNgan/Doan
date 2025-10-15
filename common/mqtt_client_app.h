#ifndef MQTT_CLIENT_APP_H
#define MQTT_CLIENT_APP_H

#include "esp_err.h"

void mqtt_app_start(void);
void mqtt_app_publish(const char *topic, const char *data, size_t len);
void mqtt_app_subscribe(const char *topic);

void mqtt_app_register_rpc_callback(void (*callback)(const char *data));

#endif
