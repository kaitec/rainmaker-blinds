#pragma once
#include <stdint.h>
#include <stdbool.h>

void rainmaker_node_init(void);
void rainmaker_device_init(void);

esp_err_t write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
            const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx);