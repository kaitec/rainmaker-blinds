#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_devices.h>
#include <esp_rmaker_schedule.h>
#include <esp_rmaker_scenes.h>

#include <app_wifi.h>
#include <app_insights.h>
#include "esp_diagnostics_system_metrics.h"
#include "esp_rmaker_utils.h"
#include "rainmaker.h"
#include "main.h"
#include "hardware.h"

void app_main()
{
    app_driver_init();

    /* Initialize NVS. */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    app_wifi_init();
    rainmaker_node_init();
    rainmaker_device_init();
    esp_rmaker_ota_enable_default();
    esp_rmaker_timezone_service_enable();
    esp_rmaker_schedule_enable();
    esp_rmaker_scenes_enable();
    app_insights_enable();
    esp_rmaker_start();

    err = app_wifi_start(POP_TYPE_RANDOM);
    if (err != ESP_OK) { // Could not start Wifi. Aborting!!!
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }
}
