#ifndef UART_HANDLER
#define UART_HANDLER

#include <stddef.h>
#include <stdint.h>

void uart_init(void);
int uart_read(uint8_t *byte);
int uart_write(void *msg, size_t size);
void uart_send_command(char *cmd, int delay_ms);
void tek_checkhealth(void);
void uart_task_init(void *pvParameters);

#endif // !UART_HANDLER
