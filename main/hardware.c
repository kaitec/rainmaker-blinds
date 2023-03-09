#include <iot_button.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h> 
#include <esp_rmaker_standard_params.h> 

#include <stdint.h>
#include <stdbool.h>
#include <esp_system.h>
#include <driver/gpio.h>
#include <app_reset.h>
#include <ws2812_led.h>
#include "rainmaker.h"
#include "hardware.h"
#include "enocean.h"
#include "INA226.h"
#include "flash.h"
#include "motor.h"

esp_timer_handle_t fast_timer; //  1 ms
esp_timer_handle_t slow_timer; // 10 ms
uint16_t tim_count=0;

void IRAM_ATTR gpio_isr_handler(void* arg)
{
    motor_feedback = gpio_get_level(MOTOR_FB);
}

void slow_timer_callback(void *priv) // 10 ms
{
   if(motor_start) motor_handler();

   tim_count++;
   if(tim_count>100)
   {
     rmaker_voltage_update(INA226_get_voltage());
     rmaker_current_update(INA226_get_current());
     tim_count=0;
   }

   if(gpio_get_level(BUTTON)==0)
   {
     run_enocean_connection_task();
   }
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

void led_green_blink(void)
{
    for(int i=0; i<3; i++)
    {
        gpio_set_level(LED_G, LED_ON); 
        vTaskDelay(100/portTICK_PERIOD_MS);
        gpio_set_level(LED_G, LED_OFF);
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
}

void hardware_init()
{
    gpio_init();
    timer_init();
    i2c_init();
    INA226_init();
    INA226_calibrate();
    led_green_blink();
}
