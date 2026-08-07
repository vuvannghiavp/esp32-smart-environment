idf_component_register(SRCS "mqtt.c"
                    INCLUDE_DIRS "."
                    REQUIRES web_server nvs_flash esp-tls esp_eth configurate )
