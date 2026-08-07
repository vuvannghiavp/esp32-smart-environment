idf_component_register(
                SRCS "conf.c"
                INCLUDE_DIRS "."
                REQUIRES esp_driver_gpio  esp_driver_ledc  BH1750 DHT22)