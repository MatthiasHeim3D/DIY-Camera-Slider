/*
CameraSlider - Camera Control
Description: This file contains the main loop for the camera control.
*/

#include <Arduino.h>
#include "SliderConfig.h"

// Internal state variables
extern CameraState_t cameraState;

void CameraControl_tick();

struct PinDelayParameters
{
    int pin;
    int delayTime;
};


void setPinHigh(void *parameter);


void setPinHighAsync(int pin, int delayTime);

void CameraControl_ReleaseShutter();