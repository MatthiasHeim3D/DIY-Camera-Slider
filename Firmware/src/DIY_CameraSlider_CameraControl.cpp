/*
CameraSlider - Camera Control
Description: This file contains the main loop for the camera control.
*/

#include <Arduino.h>
#include "DIY_CameraSlider_CameraControl.h"

// Internal state variables
CameraState_t cameraState = CAMERA_IDLE;

void CameraControl_tick()
{
    switch(cameraState)
    {
        case CAMERA_IDLE:
            break;

        case CAMERA_SHUTTER_FOCUS:
            break;

        case CAMERA_SHUTTER_FOCUSING:
            break;

        case CAMERA_SHUTTER_RELEASE:
            CameraControl_ReleaseShutter();
            break;

        case CAMERA_SHUTTER_RELEASING:
            break;

        default:
            break;
    }
}

void setPinHigh(void *parameter)
{
    PinDelayParameters *pinDelayParams = (PinDelayParameters *)parameter;
    digitalWrite(pinDelayParams->pin, HIGH);
    vTaskDelay(pdMS_TO_TICKS(pinDelayParams->delayTime));
    digitalWrite(pinDelayParams->pin, LOW);
    vTaskDelete(NULL); // Delete the task after it is done
}

void setPinHighAsync(int pin, int delayTime)
{
    static PinDelayParameters pinDelayParams;
    pinDelayParams.pin = pin;
    pinDelayParams.delayTime = delayTime;
    xTaskCreate(setPinHigh, "Set Pin High", 1000, &pinDelayParams, 1, NULL);
}

void CameraControl_ReleaseShutter()
{
    // Focus
    setPinHighAsync(PIN_SHUTTER, 300);
    cameraState = CAMERA_SHUTTER_RELEASING;

    Serial.println("Shutter released.");
}