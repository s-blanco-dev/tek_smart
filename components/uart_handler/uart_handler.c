#include "uart_handler.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_rom_serial_output.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define UART_TX_PIN 6
#define UART_RX_PIN 7
#define UART_NUM UART_NUM_1
#define UART_BUF_SIZE 1024

static const char *TAG = "UART_HANDLER";

void uart_init(void) {
  uart_config_t uart_config = {
      .baud_rate = 9600,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .rx_flow_ctrl_thresh = 122,
  };

  ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));

  ESP_ERROR_CHECK(
      uart_driver_install(UART_NUM, UART_BUF_SIZE, UART_BUF_SIZE, 10, NULL, 0));
  ESP_ERROR_CHECK(uart_set_pin(UART_NUM, UART_TX_PIN, UART_RX_PIN,
                               UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
  ESP_LOGI(TAG, "UART initialized");
}

void tek_checkhealth(void) {
  uart_flush_input(UART_NUM);

  const char *cmd = "ID?\n";
  uart_write_bytes(UART_NUM, cmd, strlen(cmd));
  ESP_LOGI(TAG, "SENT %s", cmd);

  uint8_t data[UART_BUF_SIZE];
  int len =
      uart_read_bytes(UART_NUM, data, UART_BUF_SIZE - 1, pdMS_TO_TICKS(1000));

  if (len > 0) {
    data[len] = '\0';
    ESP_LOGI(TAG, "RECEIVED (%d bytes): %s", len, data);
  } else {
    ESP_LOGE(TAG, "NO RESPONSE FROM DEVICE (Timeout).");
  }
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
  buf[len]     = '\r';
  buf[len + 1] = '\n';

  // 4. Enviar los bytes exactos por el puerto serie
  uart_write_bytes(UART_NUM, buf, len + 2);
}

void uart_send_scpi_cmd(const char *cmd, size_t len) {
  char buf[UART_BUF_SIZE];

  // Copiar el comando y asegurar que termine en \r\n\0
  int formatted_len = snprintf(buf, sizeof(buf), "%.*s\n", (int)len, cmd);

  ESP_LOGI(TAG, "SENT TO TEK: %s", buf);
  uart_write_bytes(UART_NUM, buf, formatted_len);
}

void uart_send_command(char *cmd, int delay_ms) {
  uart_flush_input(UART_NUM);

  uart_write(cmd, strlen(cmd));

  uint8_t data[UART_BUF_SIZE];
  int len = uart_read_bytes(UART_NUM, data, UART_BUF_SIZE - 1,
                            pdMS_TO_TICKS(1000));

  if (len > 0) {
    data[len] = '\0';
    ESP_LOGI(TAG, "RECEIVED (%d bytes): %s", len, data);
  } else {
    ESP_LOGE(TAG, "NO RESPONSE FROM DEVICE (Timeout).");
  }
}

void uart_task_init(void *pvParameters) { uart_init(); }
