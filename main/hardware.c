#include <iot_button.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h> 
#include <esp_rmaker_standard_params.h> 

#include <stdint.h>
#include <stdbool.h>
#include <driver/gpio.h>
#include <app_reset.h>
#include <ws2812_led.h>
#include "hardware.h"
#include "motor.h"

esp_timer_handle_t fast_timer; //  1 ms
esp_timer_handle_t slow_timer; // 10 ms

void IRAM_ATTR gpio_isr_handler(void* arg)
{
    motor_feedback = gpio_get_level(MOTOR_FB);
}

void slow_timer_callback(void *priv) // 10 ms
{
   HardmainTask();
}

void fast_timer_callback(void *priv) // 1 ms
{
    timer_function();
}

void timer_init(void)
{
    esp_timer_create_args_t slow_timer_config = {
        .callback = slow_timer_callback,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "slow"};
    esp_timer_create(&slow_timer_config, &slow_timer);

    esp_timer_create_args_t fast_timer_config = {
        .callback = fast_timer_callback,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "fast"};
    esp_timer_create(&fast_timer_config, &fast_timer);

    esp_timer_start_periodic(slow_timer, 10000U); 
    esp_timer_start_periodic(fast_timer,  1000U); 
}

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

    gpio_set_level(LED_R, 1); gpio_set_level(UP_DIR, 0);
    gpio_set_level(LED_G, 1); gpio_set_level(DOWN_DIR, 0);

    // GPIO IN INIT
    io_conf.intr_type = GPIO_INTR_ANYEDGE;// GPIO_INTR_NEGEDGE // GPIO_INTR_POSEDGE // GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(MOTOR_FB, gpio_isr_handler, (void*) MOTOR_FB);
}

void hardware_init()
{
    gpio_init();
    timer_init();

    // ws2812_led_init();
    // app_reset_button_register(app_reset_button_create(RESET_BUTTON, RESET_ACTIVE_LEVEL),
    //                            WIFI_RESET_BUTTON_TIMEOUT, FACTORY_RESET_BUTTON_TIMEOUT);
}

esp_err_t app_fan_set_power(bool power)
{
    // if (power) 
    // {
    //     gpio_set_level(OUTPUT_GPIO, 1);
    // } 
    // else 
    // {
    //     gpio_set_level(OUTPUT_GPIO, 0);
    // }
    return ESP_OK;
}

esp_err_t app_fan_set_speed(uint8_t height, uint8_t angle)
{
    //return ws2812_led_set_rgb(0, height, angle*8);
    return ESP_OK;
}
