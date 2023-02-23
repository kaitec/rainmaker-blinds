#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <app_wifi.h>
#include "backend_if.h"
#include "rainmaker.h"
#include "hardware.h"
#include "flash.h"
#include "motor.h"

void app_main()
{
    hardware_init();
    flash_init();
    motor_init();
    app_wifi_init();
    rainmaker_init();

    esp_err_t err;
    err = app_wifi_start(POP_TYPE_RANDOM);
    if (err != ESP_OK) { // Could not start Wifi. Aborting!!!
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }
}
