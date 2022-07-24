#include <iot_button.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h> 
#include <esp_rmaker_standard_params.h> 

#include <stdint.h>
#include <stdbool.h>

#include <app_reset.h>
#include <ws2812_led.h>
#include "hardware.h"

void gpio_init(void)
{
    // GPIO OUT INIT
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    //gpio_set_level(LED_STATUS_R, 1);

    // GPIO IN INIT
    io_conf.intr_type = GPIO_INTR_ANYEDGE;// GPIO_INTR_NEGEDGE // GPIO_INTR_POSEDGE // GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //gpio_isr_handler_add(HALL_IN, gpio_isr_handler, (void*) HALL_IN);
}

esp_err_t app_fan_set_power(bool power)
{
    if (power) 
    {
        gpio_set_level(OUTPUT_GPIO, 1);
    } 
    else 
    {
        gpio_set_level(OUTPUT_GPIO, 0);
    }
    return ESP_OK;
}

esp_err_t app_fan_set_speed(uint8_t height, uint8_t angle)
{
    return ws2812_led_set_rgb(0, height, angle*8);
}

void hardware_init()
{
    ws2812_led_init();
    app_reset_button_register(app_reset_button_create(RESET_BUTTON, RESET_ACTIVE_LEVEL),
                               WIFI_RESET_BUTTON_TIMEOUT, FACTORY_RESET_BUTTON_TIMEOUT);

    /* Configure GPIO */
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 1,
    };
    io_conf.pin_bit_mask = ((uint64_t)1 << OUTPUT_GPIO);
    /* Configure the GPIO */
    gpio_config(&io_conf);
}
