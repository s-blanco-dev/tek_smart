#ifndef UART_HANDLER
#define UART_HANDLER

void uart_init(void);
void tek_checkhealth(void);
void uart_task_init(void *pvParameters);

#endif // !UART_HANDLER
