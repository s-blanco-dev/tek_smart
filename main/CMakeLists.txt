idf_component_register(SRCS "tek_smart.c"
                    REQUIRES uart_handler mqtt_handler wifi_manager nvs http_handler
                    INCLUDE_DIRS ".")
