#pragma once
#include <stdint.h>
#include <stdbool.h>

#define DEFAULT_POWER       true

void app_driver_init(void);
esp_err_t app_fan_set_power(bool power);
esp_err_t app_fan_set_speed(uint8_t height, uint8_t angle);
