#ifndef UART_HANDLER
#define UART_HANDLER

#include "driver/uart.h"
#include <stddef.h>
#include <stdint.h>

void uart_init(void);
int uart_read(uint8_t *byte);
void uart_write(const char *data, size_t len);
void uart_send_command(char *cmd);
void tek_checkhealth(void);
void uart_task(void *pvParameters);

typedef void (*uart_on_data_receive)(char* msg);
void uart_set_receive_callback(void (*func)(char* msg));

typedef struct uart_handler_config_s {
  uart_config_t *uart_config;
  uart_on_data_receive receive_callback;
} uart_handler_config_t;

void set_uart_handler_config(uart_handler_config_t *config);

#endif // !UART_HANDLER
