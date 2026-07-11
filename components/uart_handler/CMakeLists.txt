idf_component_register(SRCS "uart_handler.c"
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS ""
                       PRIV_REQUIRES "driver" esp_driver_uart
                     )
