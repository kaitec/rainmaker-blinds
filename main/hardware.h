#ifndef HARDWARE_H_
#define HARDWARE_H_

#pragma once
#include <stdint.h>
#include <stdbool.h>

#define PCB_REV  10

#if(PCB_REV==10)
#define LED_R          32 // RGB LED
#define LED_G          33 // RGB LED
//#define LED_B          33 // RGB LED
#define HALL_IN        34 // HALL sensor
#define UP_DIR          2 // MOTOR drive up
#define DOWN_DIR        4 // MOTOR drive down
#define BUTTON         35 // Settings button
#define I2C_SDA        14
#define I2C_SCL        15
#define ENOCEAN_TX     12
#define ENOCEAN_RX     13
#define MOTOR_FB       HALL_IN
#define WIND_SENSOR    ENOCEAN_TX
#define GPIO_OUTPUT_PIN_SEL ((1ULL<<LED_R) | (1ULL<<LED_G) | (1ULL<<UP_DIR) | (1ULL<<DOWN_DIR))
#define GPIO_INPUT_PIN_SEL  ((1ULL<<HALL_IN) | (1ULL<<ENOCEAN_RX) | (1ULL<<BUTTON) | (1ULL<<WIND_SENSOR))

#elif(PCB_REV==8)
#define LED_R               32
#define LED_G               33
#define MOTOR_FB            34
#define UP_DIR               2
#define DOWN_DIR             4
#define BUTTON              35
#define GPIO_OUTPUT_PIN_SEL ((1ULL<<LED_R) | (1ULL<<LED_G) | (1ULL<<UP_DIR) | (1ULL<<DOWN_DIR))
#define GPIO_INPUT_PIN_SEL  ((1ULL<<MOTOR_FB) | (1ULL<<BUTTON))

#elif(PCB_REV==4)
#define LED_R               32
#define LED_G               33
#define UP_DIR               5
#define DOWN_DIR            18
#define MOTOR_FB             4
#define BUTTON              34
#define GPIO_OUTPUT_PIN_SEL ((1ULL<<LED_R) | (1ULL<<LED_G) | (1ULL<<UP_DIR) | (1ULL<<DOWN_DIR))
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
void led_blink(void);
void hardware_init(void);

#endif /* HARDWARE_H_ */