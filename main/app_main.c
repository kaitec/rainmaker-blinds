/* Fan Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

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

#include "app_priv.h"

static const char *TAG = "app_main";

uint8_t height, angle;

/* Callback to handle commands received from the RainMaker cloud */
static esp_err_t write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
            const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    if (ctx) {
        ESP_LOGI(TAG, "Received write request via : %s", esp_rmaker_device_cb_src_to_str(ctx->src));
    }
    char *device_name = esp_rmaker_device_get_name(device);
    char *param_name = esp_rmaker_param_get_name(param);
    if (strcmp(param_name, "height") == 0) {
        ESP_LOGI(TAG, "Received height = %d", val.val.i);
        height=val.val.i;
    } else if (strcmp(param_name, "angle") == 0) {
        ESP_LOGI(TAG, "Received angle = %d", val.val.i);
        angle=val.val.i;
    } else {
        /* Silently ignoring invalid params */
        return ESP_OK;
    }
    app_fan_set_speed(height, angle);
    esp_rmaker_param_update_and_report(param, val);
    return ESP_OK;
}

void app_main()
{
    /* Initialize Application specific hardware drivers and
     * set initial state.
     */
    app_driver_init();

    /* Initialize NVS. */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    /* Initialize Wi-Fi. Note that, this should be called before esp_rmaker_node_init()
     */
    app_wifi_init();
    
    /* Initialize the ESP RainMaker Agent.
     * Note that this should be called after app_wifi_init() but before app_wifi_start()
     * */
    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = false,
    };
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "Smart Blins Device", "Two channel");
    if (!node) {
        ESP_LOGE(TAG, "Could not initialise node. Aborting!!!");
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }

    /********** Blinds devices ****************/
    esp_rmaker_device_t *device = esp_rmaker_device_create("Blinds", "esp.device.blinds-external", NULL);
    esp_rmaker_device_add_param(device, esp_rmaker_param_create("name", NULL, esp_rmaker_str("Blind Name"), PROP_FLAG_READ | PROP_FLAG_WRITE));
    esp_rmaker_device_add_cb(device, write_cb, NULL);
    /********** Blinds param height ***********/
    esp_rmaker_param_t *height_param = esp_rmaker_param_create("height", "esp.param.range", esp_rmaker_int(0), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(height_param, "esp.ui.slider");
    esp_rmaker_param_add_bounds(height_param, esp_rmaker_int(0), esp_rmaker_int(100), esp_rmaker_int(1));
    esp_rmaker_device_add_param(device, height_param);
    /********** Blinds param angle ***********/
    esp_rmaker_param_t *angle_param = esp_rmaker_param_create("angle", "esp.param.range", esp_rmaker_int(0), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(angle_param, "esp.ui.slider");
    esp_rmaker_param_add_bounds(angle_param, esp_rmaker_int(0), esp_rmaker_int(12), esp_rmaker_int(1));
    esp_rmaker_device_add_param(device, angle_param);
    /* Generic Mode Parameter */
     esp_rmaker_param_t *mode = esp_rmaker_param_create("Settings", "esp.param.mode", esp_rmaker_str("None"), PROP_FLAG_READ | PROP_FLAG_WRITE);
     static const char *valid_strs[] = {"None", "Remote connect", "Motor calibration"};
     esp_rmaker_param_add_valid_str_list(mode, valid_strs, 3);
     esp_rmaker_param_add_ui_type(mode, "esp.ui.dropdown");
     esp_rmaker_device_add_param(device, mode);
    /********** Add device to Node ***********/
    esp_rmaker_node_add_device(node, device);
    /************** END **********************/

    /* Enable OTA */
    esp_rmaker_ota_enable_default();

    /* Enable timezone service which will be require for setting appropriate timezone
     * from the phone apps for scheduling to work correctly.
     * For more information on the various ways of setting timezone, please check
     * https://rainmaker.espressif.com/docs/time-service.html.
     */
    esp_rmaker_timezone_service_enable();

    /* Enable scheduling. */
    esp_rmaker_schedule_enable();

    /* Enable Scenes */
    esp_rmaker_scenes_enable();

    /* Enable Insights. Requires CONFIG_ESP_INSIGHTS_ENABLED=y */
    app_insights_enable();

    /* Start the ESP RainMaker Agent */
    esp_rmaker_start();

    /* Start the Wi-Fi.
     * If the node is provisioned, it will start connection attempts,
     * else, it will start Wi-Fi provisioning. The function will return
     * after a connection has been successfully established
     */
    err = app_wifi_start(POP_TYPE_RANDOM);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Could not start Wifi. Aborting!!!");
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }
}
