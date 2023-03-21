#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "sdkconfig.h"
#include <stdio.h>
#include <math.h>
#include "hardware.h"
#include "INA226.h"

float currentLSB;

esp_err_t i2c_read(uint8_t *data_rd, size_t size)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ESP_SLAVE_ADDR << 1) | READ_BIT, ACK_CHECK_EN);
    if (size > 1) {
        i2c_master_read(cmd, data_rd, size - 1, ACK_VAL);
    }
    i2c_master_read_byte(cmd, data_rd + size - 1, NACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 10 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t i2c_write_reg(uint8_t reg)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ESP_SLAVE_ADDR << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 10 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t i2c_write_16(uint8_t reg, uint16_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ESP_SLAVE_ADDR << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, data>>8, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, data, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 10 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

void i2c_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_SDA;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_SCL;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    conf.clk_flags = 0;          /*!< Optional, you can use I2C_SCLK_SRC_FLAG_* flags to choose i2c source clock here. */
    i2c_param_config(i2c_master_port, &conf);
    i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

void INA226_init(void)
{
    uint8_t data[2] = {0};
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_write_reg(INA226_REG_CONFIG);
    i2c_read(data,2);    
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 10 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
}

void INA226_calibrate(void)
{
    uint16_t calibrationValue;
    currentLSB = MAX_CUR/pow(2,15);
    calibrationValue = (uint16_t)((0.00512) / (currentLSB * RSHUNT));

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_write_16(INA226_REG_CALIBRATION, calibrationValue);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 10 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
}

uint16_t INA226_get_data(uint8_t reg)
{
  uint8_t data[] = {0,0};
  uint16_t result;

  i2c_write_reg(reg);
  i2c_read(data,2);

  result = data[0];
  result <<= 8;
  result |= data[1];

  return (uint16_t)result;
}

float INA226_get_voltage(void)
{
  float raw_volt=0;
  float voltage=0;

  raw_volt  = INA226_get_data(INA226_REG_BUSVOLTAGE);
  raw_volt *= VOLTAGE_RESOLUTION;
  raw_volt *= RESISTOR_DIVIDER_FACTOR;
  // 123.4
  int i=0;
  raw_volt *= 10;
  i = raw_volt;
  voltage = (float) i / 10;

  return voltage;
}

float INA226_get_current(void)
{
  int16_t raw=0;
  float raw_curr=0;
  float current=0;

  raw  = INA226_get_data(INA226_REG_CURRENT);
  if(raw<0) raw=0;
  raw_curr = raw * currentLSB;
  // 12.34
  int i=0;
  raw_curr *= 100;
  i = raw_curr;
  current = (float) i / 100;
  
  return current;
}

float INA226_get_power(void)
{
  float si=0, su=0;
  float power=0;

  su = INA226_get_voltage();
  si = INA226_get_current();

  power = su*si;

  return power;
}

double generation=0;

void counting_generation()
{
  generation += INA226_get_power();
}

void reset_integrated_generation()
{
  generation=0;
}

double get_integrated_generation()
{
  return (float)generation;
}