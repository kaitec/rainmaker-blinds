#ifndef HARDWARE_H_
#define HARDWARE_H_

#pragma once
#include <stdint.h>
#include <stdbool.h>

#define PCB_REV   D4//1

#if(PCB_REV==1)
#define RESET_BUTTON        0
#define RESET_ACTIVE_LEVEL  0
#define OUTPUT_GPIO         2
#define GPIO_OUTPUT_PIN_SEL ((1ULL<<OUTPUT_GPIO))
#define GPIO_INPUT_PIN_SEL  ((1ULL<<RESET_BUTTON))
#elif(PCB_REV==D4)
#define LED_R               17
#define LED_G               16
#define LED_B               15
#define UP_DIR              13
#define DOWN_DIR            12
#define MOTOR_FB            14
#define BUTTON               0
#define GPIO_OUTPUT_PIN_SEL ((1ULL<<LED_R) | (1ULL<<LED_G) | (1ULL<<LED_B) | (1ULL<<UP_DIR) | (1ULL<<DOWN_DIR))
#define GPIO_INPUT_PIN_SEL  ((1ULL<<MOTOR_FB) | (1ULL<<BUTTON))
#endif

#define DEBUG  NONE
#define NONE   0
#define RMAKER 1
#define MOTOR  2

#define ESP_INTR_FLAG_DEFAULT           0
#define WIFI_RESET_BUTTON_TIMEOUT       3
#define FACTORY_RESET_BUTTON_TIMEOUT    10


void gpio_init(void);
void timer_init(void);
void hardware_init(void);
esp_err_t app_fan_set_power(bool power);
esp_err_t app_fan_set_speed(uint8_t height, uint8_t angle);

#endif /* HARDWARE_H_ */