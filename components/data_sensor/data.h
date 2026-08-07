#pragma once
#include <stdbool.h>
typedef struct
{
    float temp;
    float humi;
    float lux;

} sensor_data_t;

typedef struct
{
    bool relay1;
    bool relay2;
} device_state_t;
extern sensor_data_t sensor_data;
extern device_state_t device_state;