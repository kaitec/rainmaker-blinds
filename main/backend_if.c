#include "backend_if.h"
#include "hardware.h"
#include "enocean.h"
#include "INA226.h"
#include "flash.h"
#include "motor.h"

uint8_t SetPosition(uint8_t val) // 0-100
{
    set_blind(ROLL, val);
    return 0;
}

uint8_t SetAngle(uint8_t val) // 0-180
{
    set_blind(TILT, val);
    return 0;
}

uint8_t GetPosition()
{
    return get_roll();
}

uint8_t GetAngle()
{
    return get_angle();
}

uint8_t CalibratePosition()
{
    motor_reset();
    return 0;
}

uint8_t EnoceanConnection()
{
    run_enocean_connection_task();
    return 0;
}

float GetVoltage()
{
    return INA226_get_voltage();
}

float GetCurrent()
{
    return INA226_get_current();
}

float GetPower()
{
    return INA226_get_power();
}

double GetCurrentIntegratedGeneration()
{
    return get_integrated_generation();
}

void ResetCurrentIntegratedGeneration()
{
    reset_integrated_generation();
}