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
#include "main.h"

esp_rmaker_device_t *rmaker_device;

/* Callback to handle commands received from the RainMaker cloud */
esp_err_t write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
            const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    if (ctx) {
        if(DEBUG==RMAKER) ESP_LOGI(__func__, "Received write request via : %s", esp_rmaker_device_cb_src_to_str(ctx->src));
    }
    //char *device_name = esp_rmaker_device_get_name(device);
    char *param_name = esp_rmaker_param_get_name(param);
    if (strcmp(param_name, "height") == 0) 
    {
        if(DEBUG==RMAKER) ESP_LOGI(__func__, "Received height = %d", val.val.i);
        //height=val.val.i;
    } 
    else if (strcmp(param_name, "angle") == 0) 
    {
        if(DEBUG==RMAKER) ESP_LOGI(__func__, "Received angle = %d", val.val.i);
        //angle=val.val.i;
    } 
    else 
    {
        /* Silently ignoring invalid params */
        return ESP_OK;
    }
    //app_fan_set_speed(height, angle);
    //esp_rmaker_param_update_and_report(param, val);
    return ESP_OK;
}

esp_rmaker_node_t *node;

void rainmaker_node_init(void)
{
    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = false,
    };

    node = esp_rmaker_node_init(&rainmaker_cfg, "Smart Blins Device", "Two channel");

    if (!node) {
        if(DEBUG==RMAKER) ESP_LOGE(__func__, "Could not initialise node. Aborting!!!");
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }
}

void rainmaker_device_init(void)
{
	/********** Blinds devices ****************/
    rmaker_device = esp_rmaker_device_create("Blinds", "esp.device.blinds-external", NULL);
    esp_rmaker_device_add_cb(rmaker_device, write_cb, NULL);
    /********** Blinds param height ***********/
    esp_rmaker_param_t *height_param = esp_rmaker_param_create("height", "esp.param.range", esp_rmaker_int(0), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(height_param, "esp.ui.slider");
    esp_rmaker_param_add_bounds(height_param, esp_rmaker_int(0), esp_rmaker_int(100), esp_rmaker_int(1));
    esp_rmaker_device_add_param(rmaker_device, height_param);
    /********** Blinds param angle ***********/
    esp_rmaker_param_t *angle_param = esp_rmaker_param_create("angle", "esp.param.range", esp_rmaker_int(0), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(angle_param, "esp.ui.slider");
    esp_rmaker_param_add_bounds(angle_param, esp_rmaker_int(0), esp_rmaker_int(12), esp_rmaker_int(1));
    esp_rmaker_device_add_param(rmaker_device, angle_param);
    /* Generic Mode Parameter */
     // esp_rmaker_param_t *mode = esp_rmaker_param_create("Mode", "esp.param.mode", esp_rmaker_str("None"), PROP_FLAG_READ | PROP_FLAG_WRITE);
     // static const char *valid_strs[] = {"None", "Remote connect", "Motor calibration"};
     // esp_rmaker_param_add_valid_str_list(mode, valid_strs, 3);
     // esp_rmaker_param_add_ui_type(mode, "esp.ui.dropdown");
     // esp_rmaker_device_add_param(device, mode);
    /********** Add device to Node ***********/
    esp_rmaker_node_add_device(node, rmaker_device);
    /************** END **********************/
}