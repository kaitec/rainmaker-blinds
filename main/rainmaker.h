#pragma once
#include <stdint.h>
#include <stdbool.h>

void rainmaker_node_init(void);
void rainmaker_device_init(void);
void rainmaker_init(void);

void rmaker_roll_update(uint8_t val);
void rmaker_angle_update(uint8_t val);
void rmaker_voltage_update(float val);
void rmaker_current_update(float val);