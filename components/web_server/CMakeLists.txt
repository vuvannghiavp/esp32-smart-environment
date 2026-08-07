idf_component_register(
                SRCS "web_server.c"
                INCLUDE_DIRS "."
                REQUIRES DHT22 esp_event esp_http_server json driver esp_wifi esp_timer BH1750 configurate
                EMBED_TXTFILES "../../main/dashboard.html")