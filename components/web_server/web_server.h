#pragma once
#include <stdint.h>
#include "esp_event.h"
#include "zh_bh1750.h" 
#include "conf.h"
extern const uint8_t dashboard_html_start[] asm("_binary_dashboard_html_start");
extern const uint8_t dashboard_html_end[] asm("_binary_dashboard_html_end");
void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data);
void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data);