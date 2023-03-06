#include "backend_if.h"
#include "hardware.h"
#include "enocean.h"
#include "INA226.h"
#include "flash.h"
#include "motor.h"

int32_t SetPosition(uint8_t val) // 0-100
{
    set_blind(ROLL, val);
    return 0;
}

int32_t SetAngle(uint8_t val) // 0-180
{
    set_blind(TILT, val);
    return 0;
}

int32_t GetPosition()
{
    return get_roll();
}

int32_t GetAngle()
{
    return get_angle();
}

int32_t CalibratePosition()
{
    motor_reset();
    return 0;
}

int32_t SetEnocean()
{
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