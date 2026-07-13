#include <stdio.h>
#include "uart_handler.h"

void app_main(void)
{
  uart_init();
  tek_checkhealth();
}


