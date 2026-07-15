#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "http_handler.h"
#include "mqtt_handler.h"
#include "nvs.h"
#include "soc/clk_tree_defs.h"
#include "uart_handler.h"
#include "wifi_manager.h"
#include <stdio.h>

#define BROKER_URI "mqtt://mqtt.portadomus.casa:1883"
#define TOPIC "tek/command"
const char *TAG = "TEK_SMART";

void the_callback(const char *topic, int topic_len, char *data, int data_len) {
  ESP_LOGI(TAG, "Got message in: %.*s", topic_len, topic);

  // 1. Crear un buffer seguro para terminar la cadena de MQTT en '\0'
  char safe_cmd[128];
  if (data_len >= sizeof(safe_cmd)) {
    data_len = sizeof(safe_cmd) - 1;
  }
  memcpy(safe_cmd, data, data_len);
  safe_cmd[data_len] = '\0'; // Asegurar el terminador nulo

  ESP_LOGI(TAG, "Processing safe command: %s", safe_cmd);

  // 2. Enviar SOLAMENTE el comando de MQTT (sin encadenar tek_checkhealth)
  uart_send_command(safe_cmd);
}

void receive_callback(char *msg) {
  ESP_LOGI(TAG, "Got TEK Ressponse: %s", msg);
  mqtt_publish("tek/response", msg, 2, 0);
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

  uart_config_t uart_config = {.baud_rate = 19200,
                               .data_bits = UART_DATA_8_BITS,
                               .parity = UART_PARITY_DISABLE,
                               .stop_bits = UART_STOP_BITS_1,
                               .source_clk = UART_SCLK_DEFAULT,
                               .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

  uart_handler_config_t handler_config = {
      .uart_config = &uart_config,
      .receive_callback = receive_callback,
  };

  set_uart_handler_config(&handler_config);
  uart_init();

  vTaskDelay(pdMS_TO_TICKS(1000));
  tek_checkhealth();
  while (1) {
    // Tu loop principal puede hacer otras cosas aquí (atender Wi-Fi, MQTT, LED
    // status) sin preocuparse por quedarse colgado esperando por la UART.
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}
