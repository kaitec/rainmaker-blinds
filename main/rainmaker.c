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
#include "hardware.h"
#include "rainmaker.h"
#include "enocean.h"
#include "flash.h"
#include "motor.h"

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
        set_blind(ROLL, val.val.i);
    } 
    else if (strcmp(param_name, "angle") == 0) 
    {
        set_blind(TILT, val.val.i * 15);
    } 
    else if (strcmp(param_name, "Mode") == 0) 
    {
        ESP_LOGI(__func__, "Mode: %s", val.val.s);

        if (strcmp(val.val.s, "EnConnect") == 0) run_enocean_connection_task();
        if (strcmp(val.val.s, "Calibration") == 0) motor_reset();
    } 
    else 
    {
        return ESP_OK;
    }
    esp_rmaker_param_update_and_report(param, val);
    return ESP_OK;
}

void rmaker_roll_update(uint8_t val)
{
    esp_rmaker_param_update_and_report(
            esp_rmaker_device_get_param_by_name(rmaker_device, "height"),
            esp_rmaker_int(val));
}

void rmaker_angle_update(uint8_t val)
{
    esp_rmaker_param_update_and_report(
            esp_rmaker_device_get_param_by_name(rmaker_device, "angle"),
            esp_rmaker_int(val));   
}

void rmaker_voltage_update(float val)
{
    esp_rmaker_param_update_and_report(
            esp_rmaker_device_get_param_by_name(rmaker_device, "voltage"),
            esp_rmaker_float(val));   
}

void rmaker_current_update(float val)
{
    esp_rmaker_param_update_and_report(
            esp_rmaker_device_get_param_by_name(rmaker_device, "current"),
            esp_rmaker_float(val));   
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
    esp_rmaker_param_t *height_param = esp_rmaker_param_create("height", "esp.param.blinds-position", esp_rmaker_int(100), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(height_param, "esp.ui.slider");
    esp_rmaker_param_add_bounds(height_param, esp_rmaker_int(0), esp_rmaker_int(100), esp_rmaker_int(1));
    esp_rmaker_device_add_param(rmaker_device, height_param);
    /********** Blinds param angle ***********/
    esp_rmaker_param_t *angle_param = esp_rmaker_param_create("angle", "esp.param.blinds-position", esp_rmaker_int(0), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(angle_param, "esp.ui.slider");
    esp_rmaker_param_add_bounds(angle_param, esp_rmaker_int(0), esp_rmaker_int(12), esp_rmaker_int(1));
    esp_rmaker_device_add_param(rmaker_device, angle_param);
    /********** Blinds param voltage ***********/
    esp_rmaker_param_t *voltage_param = esp_rmaker_param_create("voltage", "esp.param.temperature", esp_rmaker_float(0), PROP_FLAG_READ);
    esp_rmaker_device_add_param(rmaker_device, voltage_param);
    /********** Blinds param current ***********/
    esp_rmaker_param_t *current_param = esp_rmaker_param_create("current", "esp.param.temperature", esp_rmaker_float(0), PROP_FLAG_READ);
    esp_rmaker_device_add_param(rmaker_device, current_param);
    /********** Blinds param mode ***********/
     esp_rmaker_param_t *mode = esp_rmaker_param_create("Mode", "esp.param.mode", esp_rmaker_str("None"), PROP_FLAG_READ | PROP_FLAG_WRITE);
     static const char *valid_strs[] = {"None", "EnConnect", "Calibration"};
     esp_rmaker_param_add_valid_str_list(mode, valid_strs, 3);
     esp_rmaker_param_add_ui_type(mode, "esp.ui.dropdown");
     esp_rmaker_device_add_param(rmaker_device, mode);
    /********** Add device to Node ***********/
    esp_rmaker_node_add_device(node, rmaker_device);
    /************** END **********************/
}

void rainmaker_init(void)
{
	rainmaker_node_init();
    rainmaker_device_init();
    esp_rmaker_ota_enable_default();
    esp_rmaker_timezone_service_enable();
    esp_rmaker_schedule_enable();
    esp_rmaker_scenes_enable();
    app_insights_enable();
    esp_rmaker_start();
}