#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <app_wifi.h>
#include "rainmaker.h"
#include "hardware.h"

void app_main()
{
    hardware_init();

    /* Initialize NVS. */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    app_wifi_init();
    rainmaker_init();

    err = app_wifi_start(POP_TYPE_RANDOM);
    if (err != ESP_OK) { // Could not start Wifi. Aborting!!!
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }
}
