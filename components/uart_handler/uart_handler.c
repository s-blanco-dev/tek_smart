#include "uart_handler.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_rom_serial_output.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"

#define UART_TX_PIN 21
#define UART_RX_PIN 20
#define UART_NUM UART_NUM_0
#define MAX_LINE_LEN 8

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

  const int uart_buffer_size = (1024 * 2);
  ESP_ERROR_CHECK(uart_driver_install(UART_NUM, uart_buffer_size,
                                      uart_buffer_size, 10, NULL, 0));
  ESP_ERROR_CHECK(uart_set_pin(UART_NUM, UART_RX_PIN, UART_TX_PIN));
  ESP_LOGI(TAG, "UART initialized");
}

void uart_task_init(void *pvParameters) {
  uart_init();

  char line[MAX_LINE_LEN];
  int pos = 0;

  while (1) {
    uint8_t byte;

    // uso delay maximo para bloquear (se mide en RTOS ticks)
    int n = uart_read_bytes(UART_NUM, &byte, 1, 0xffffffff);

    if (n <= 0) {
      continue;
    }

    if (byte == '\n' || byte == '\r') {
      // si tengo un \n o \r y la posicion es 0, entonces estoy en una linea
      // vacia la skipeo
      if (pos == 0) {
        continue;
      }

      // si tengo un \n o \r y la posicion NO es 0, entonces estoy en el ultimo
      // caracter agrego el null terminator y reseteo la posicion a 0
      line[pos] = '\0';
      pos = 0;
    }
  }
}
