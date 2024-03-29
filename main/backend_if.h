#ifndef BACKEND_IF_H
#define BACKEND_IF_H

#include <stdint.h>

uint8_t GetPosition(); // get current blinds position %. Return 0 - fully closed, 100 - fully open.
uint8_t SetPosition(uint8_t val); // set current blinds position %. 0 - fully closed, 100 - fully open. Return 0 if ok, <0 for error codes. TODO add error description here

uint8_t GetAngle(); // get current blinds angle %. Return 0 - fully closed, 90 - fully open (horizontal).
uint8_t SetAngle(uint8_t val); // set current blinds angle %. 0 - fully closed, 90 - fully open (horizontal). Return 0 if ok, <0 for error codes. TODO add error description here

uint8_t CalibratePosition(); // calibrate/recalibrate position. Return 0 if ok, <0 for error codes. TODO add error description here
uint8_t EnoceanConnection(); // added new EnOcean button

float GetVoltage();// get current voltage in V
float GetCurrent();// get current current in A
float GetPower();  // get current power in W

double GetCurrentIntegratedGeneration();// get generation in ws (watt-second) since last call to ResetCurrentIntegratedGeneration()
void ResetCurrentIntegratedGeneration();// set current integrated generation to zero

/*
int32_t GetMotorState(); // 0 if ok, <0 for error.  -1 if overheat etc TODO add error description here

int64_t GetCurrentIntegratedGeneration();// get generation in ws (watt-second) since last call to ResetCurrentIntegratedGeneration()
void ResetCurrentIntegratedGeneration();// set current integrated generation to zero
void CurrentIntegratedGenerationCallback(int callbackFrequencyHz, float filter);//callback called typically at 10Hz to compute CurrentIntegratedGeneration and GetCurrentSmoothV() and GetCurrentSmoothI() (generation += (CurrentW-previousW)*filter). smoothV += (currentV - previousV) * filter 

int32_t GetCurrentV();// get current voltage in mV
int32_t GetCurrentI();// get current current in mA
int32_t GetCurrentW();// get current power in mW

int32_t GetCurrentSmoothV();// get current voltage in mV
int32_t GetCurrentSmoothI();// get current current in mA
int32_t GetCurrentSmoothW();// get current power in mW
*/
#endif