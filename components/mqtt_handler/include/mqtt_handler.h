#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <stdint.h>
#include <stdbool.h>

typedef void (*mqtt_on_message_cb_t)(const char *topic, int topic_len, char *data, int data_len);

// mas simple, limpio grasa de pollo
typedef struct {
    const char *broker_uri;
    const char *username;
    const char *password;
    mqtt_on_message_cb_t on_message_callback;
} mqtt_config_t;

// --- PUBLIC API ---
bool mqtt_wait_for_connection(uint32_t timeout_ms);

void mqtt_app_start(const mqtt_config_t *config);

int mqtt_publish(const char *topic, const char *payload, int qos, bool retain);

int mqtt_subscribe(const char *topic, int qos);

int mqtt_unsubscribe(const char *topic);

#endif // MQTT_HANDLER_H
