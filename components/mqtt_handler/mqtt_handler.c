#include "mqtt_handler.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "mqtt_client.h"

static const char *TAG = "MQTT_API";

static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static mqtt_on_message_cb_t s_message_cb = NULL;

static EventGroupHandle_t s_mqtt_event_group;
#define MQTT_CONNECTED_BIT BIT0

static void mqtt5_event_handler(void *handler_args, esp_event_base_t base,
                                int32_t event_id, void *event_data) {
  esp_mqtt_event_handle_t event = event_data;

  switch ((esp_mqtt_event_id_t)event_id) {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "Connected to MQTT broker");
    xEventGroupSetBits(s_mqtt_event_group, MQTT_CONNECTED_BIT);
    break;

  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "Desconectado del Broker");
    xEventGroupClearBits(s_mqtt_event_group, MQTT_CONNECTED_BIT);
    break;

  case MQTT_EVENT_DATA:
    if (s_message_cb != NULL) {
      s_message_cb(event->topic, event->topic_len, event->data,
                   event->data_len);
    }
    break;

  case MQTT_EVENT_ERROR:
    ESP_LOGE(TAG, "Error en MQTT");
    break;

  default:
    break;
  }
}

void mqtt_app_start(const mqtt_config_t *config) {
  if (s_mqtt_client != NULL) {
    ESP_LOGW(TAG, "El cliente MQTT ya está inicializado.");
    return;
  }

  s_mqtt_event_group = xEventGroupCreate();
  s_message_cb = config->on_message_callback;

  esp_mqtt_client_config_t mqtt_cfg = {
      .broker.address.uri = config->broker_uri,
      .session.protocol_ver = MQTT_PROTOCOL_V_3_1_1,
      .credentials.username = config->username,
      .credentials.authentication.password = config->password,
      .network.timeout_ms = 10000,
      .session.keepalive = 60,
      .session.disable_clean_session = false,
  };

  s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_register_event(s_mqtt_client, ESP_EVENT_ANY_ID,
                                 mqtt5_event_handler, NULL);
  esp_mqtt_client_start(s_mqtt_client);
}

bool mqtt_wait_for_connection(uint32_t timeout_ms) {
  if (s_mqtt_event_group == NULL)
    return false;

  EventBits_t bits =
      xEventGroupWaitBits(s_mqtt_event_group, MQTT_CONNECTED_BIT,
                          pdFALSE, // No limpiar el bit al salir (queremos que
                                   // siga marcado como conectado)
                          pdTRUE,  // Esperar a todos los bits (solo hay uno)
                          pdMS_TO_TICKS(timeout_ms));

  return (bits & MQTT_CONNECTED_BIT) != 0;
}

int mqtt_publish(const char *topic, const char *payload, int qos, bool retain) {
  if (s_mqtt_client == NULL)
    return -1;
  return esp_mqtt_client_publish(s_mqtt_client, topic, payload, 0, qos,
                                 retain ? 1 : 0);
}

int mqtt_subscribe(const char *topic, int qos) {
  if (s_mqtt_client == NULL)
    return -1;
  return esp_mqtt_client_subscribe(s_mqtt_client, topic, qos);
}

int mqtt_unsubscribe(const char *topic) {
  if (s_mqtt_client == NULL)
    return -1;
  return esp_mqtt_client_unsubscribe(s_mqtt_client, topic);
}
