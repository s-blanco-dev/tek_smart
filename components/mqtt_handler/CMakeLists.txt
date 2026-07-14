idf_component_register(SRCS "mqtt_handler.c"
                    PRIV_REQUIRES mqtt nvs_flash esp_netif
                       INCLUDE_DIRS "include"
                     )
