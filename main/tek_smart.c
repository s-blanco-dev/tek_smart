#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "http_handler.h"
#include "mqtt_handler.h"
#include "nvs.h"
#include "uart_handler.h"
#include "wifi_manager.h"
#include <stdio.h>

#define BROKER_URI "mqtt://mqtt.portadomus.casa:1883"
#define TOPIC "tek/command"
const char *TAG = "TEK_SMART";

void the_callback(const char *topic, int topic_len, char *data, int data_len) {
  ESP_LOGI(TAG, "Got message in: %.*s", topic_len, topic);
  ESP_LOGI(TAG, "MESSAGE: %.*s", data_len, data);
  uart_send_command(data, 1000);
}

mqtt_config_t config = {
    .broker_uri = BROKER_URI,
    .username = "TDS2MEM",
    .password = "no",
    .on_message_callback = the_callback,
};

static void network_services_task(void *pvParameters) {
  (void)pvParameters;

  wifi_manager_status_t status;

  while (true) {
    wifi_manager_get_status(&status);

    if (status.connected) {
      ESP_LOGI(TAG, "WiFi connected with IP %s. Initializing MQTT...",
               status.ip_address);
      mqtt_app_start(&config);
      if (mqtt_wait_for_connection(15000)) {
        ESP_LOGI(TAG, "Connected to broker");
        mqtt_subscribe(TOPIC, 1);
      } else {
        ESP_LOGE(TAG, "MQTT connection timeout");
      }
      vTaskDelete(NULL);
    }

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

static void app_network_init(void) {
  ESP_ERROR_CHECK(nvs_storage_init());
  ESP_ERROR_CHECK(wifi_manager_init());
  ESP_ERROR_CHECK(http_handler_start());

  ESP_LOGI(TAG, "CONNECT to %s", wifi_manager_get_ap_ssid());
  ESP_LOGI(TAG, "OPEN BROWSER IN: http://192.168.4.1");

  xTaskCreate(&network_services_task, "NETWORK_SERVICES", 4096, NULL, 1, NULL);
}

void app_main(void) {
  app_network_init();
  uart_init();
  // tek_checkhealth();
}
