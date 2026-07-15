#include "uart_handler.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_rom_serial_output.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "hal/uart_types.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define UART_TX_PIN 6
#define UART_RX_PIN 7
#define UART_BUF_SIZE 1024

#define RX_TEMP_BUF_SIZE 256
#define RX_ACCUM_BUF_SIZE 4096 // to save CURVE? response

static const uart_port_t UART_NUM = UART_NUM_1;
static const char *TAG = "UART_HANDLER";
static uart_config_t g_uart_config;
static uart_on_data_receive g_receive_callback = NULL;

static uint8_t rx_accum_buf[RX_ACCUM_BUF_SIZE];
static size_t rx_accum_len = 0;

static void uart_rx_task(void *pvParameters) {
  uint8_t tmp_buf[RX_TEMP_BUF_SIZE];
  ESP_LOGI(TAG, "UART RX Task started");

  while (1) {
    int len = uart_read_bytes(UART_NUM, tmp_buf, sizeof(tmp_buf) - 1,
                              pdMS_TO_TICKS(100));

    if (len > 0) {
      if (rx_accum_len + len < RX_ACCUM_BUF_SIZE - 1) {
        memcpy(&rx_accum_buf[rx_accum_len], tmp_buf, len);
        rx_accum_len += len;
      } else {
        ESP_LOGE(TAG, "Accumulator buffer overflow!");
        rx_accum_len = 0; // in case I die
      }
    } else {
      if (rx_accum_len > 0) {
        rx_accum_buf[rx_accum_len] = '\0';

        ESP_LOGI(TAG, "RESPONSE RECEIVED (%d bytes):", rx_accum_len);

        if (g_receive_callback != NULL) {
          g_receive_callback((char *)rx_accum_buf); // callback uuuu
        }

        rx_accum_len = 0;
      }
    }
  }
}

void uart_init(void) {
  ESP_ERROR_CHECK(uart_param_config(UART_NUM, &g_uart_config));

  ESP_ERROR_CHECK(uart_set_pin(UART_NUM, UART_TX_PIN, UART_RX_PIN,
                               UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
  ESP_ERROR_CHECK(
      uart_driver_install(UART_NUM, 2048, UART_BUF_SIZE, 0, NULL, 0));

  xTaskCreate(uart_rx_task, "uart_rx_task", 4096, NULL, 10, NULL);
  ESP_LOGI(TAG, "UART initialized");
}

void tek_checkhealth(void) {
  uart_send_command("ID?");
}

// wrapper
int uart_read(uint8_t *byte) {
  return uart_read_bytes(UART_NUM, byte, 1, portMAX_DELAY);
}

// wrapper
void uart_write(const char *data, size_t len) {
  if (data == NULL || len == 0) {
    return;
  }

  char buf[UART_BUF_SIZE];

  // Limitar longitud para reservar espacio para \r, \n y \0
  if (len > sizeof(buf) - 3) {
    len = sizeof(buf) - 3;
  }

  // 1. Copiar datos limpios
  memcpy(buf, data, len);

  // 2. Imprimir en LOG ANTES de agregar \r\n para no romper la terminal serie
  ESP_LOGI(TAG, "SENT TO TEK (%d bytes): %.*s", (int)len, (int)len, buf);

  // 3. Formatear terminadores SCPI obligatorios para Tektronix
  buf[len] = '\r';
  buf[len + 1] = '\n';

  // 4. Enviar los bytes exactos por el puerto serie
  uart_write_bytes(UART_NUM, buf, len + 2);
}

void uart_send_command(char *cmd) {
  if (cmd == NULL) {
    return;
  }
  uart_flush_input(UART_NUM);
  uart_write(cmd, strlen(cmd));
}

void set_uart_handler_config(uart_handler_config_t *config) {
  if (config == NULL)
    return;

  if (config->uart_config != NULL) {
    g_uart_config = *(config->uart_config);
  }

  if (config->receive_callback != NULL) {
    g_receive_callback = config->receive_callback;
  }
}

void uart_set_receive_callback(void (*func)(char *msg)) {
  g_receive_callback = func;
}
