/*
CameraSlider - Motor Control
Description: This file contains all the functions we use to initialize, configure and control motors
*/

#include "SliderConfig.h"
#include <FlexyStepper.h>
#include "DIY_CameraSlider_MotorControl.h"
#include "DIY_CameraSlider_CameraControl.h"

FlexyStepper stepper_slide;
FlexyStepper stepper_pan;

// Internal state variables
sliderState_t sliderState = SLIDER_IDLE;
sliderState_t prev_sliderState = SLIDER_IDLE;
bool bmotorState          = true;
bool bhomingComplete      = false;
float fSliderPos          = 0.0;
float fRotationPos        = 0.0;

float stepDist = 0;
int currentStep = 0;
int maxSteps = 0;
float startPos = 0;

// Start and stop positions
float fStartPos_Slider    = 0.0;
float fStartPos_Rotation  = 0.0;

float fEndPos_Slider      = 0.0;
float fEndPos_Rotation    = 0.0;

float fSlidingSpeed       = 0.0;
float fRotatingSpeed      = 0.0;

uint32_t slideDurationSec = 1;

// Main motor control function.
// We call this function from within our main loop to
// execute different commands based on current state
// of the CameraSlider state machine. Values/states of
// this state machine can be updated asynchronously (HTTP)
// or trough interrupts.
void CameraSlider_tick()
{
    if(sliderState != prev_sliderState)
    {
        Serial.println(sliderStateStr[sliderState]);
        prev_sliderState = sliderState;
    }

    switch(sliderState)
    {
        case SLIDER_MOTORS_OFF:
        break;

        case SLIDER_IDLE:
        break;

        case SLIDER_MOVING_TO_START:
            // Moving to start
            if((!stepper_slide.motionComplete()) || (!stepper_pan.motionComplete()))
            {
                stepper_slide.processMovement();
                stepper_pan.processMovement();
            }
            // Move complete
            else
            {
                if(slideDurationSec <=0 ) { slideDurationSec = 1; }

                // Calculate speed
                if(fEndPos_Slider >= fStartPos_Slider)
                {
                    fSlidingSpeed = (fEndPos_Slider - fStartPos_Slider) / slideDurationSec;
                }
                else
                {
                    fSlidingSpeed = (fStartPos_Slider - fEndPos_Slider) / slideDurationSec;
                }

                if(fEndPos_Rotation >= fStartPos_Rotation)
                {
                    fRotatingSpeed = (fEndPos_Rotation - fStartPos_Rotation) / (slideDurationSec);
                }
                else
                {
                    fRotatingSpeed = (fStartPos_Rotation - fEndPos_Rotation) / (slideDurationSec);
                }

                // Configure slider
                stepper_slide.setSpeedInMillimetersPerSecond(fSlidingSpeed);
                stepper_slide.setAccelerationInMillimetersPerSecondPerSecond(SliderConfig.Config.default_slider_accel);
                stepper_slide.setTargetPositionInMillimeters(fEndPos_Slider);

                // Configure pan
                stepper_pan.setSpeedInStepsPerSecond(fRotatingSpeed);
                stepper_pan.setAccelerationInStepsPerSecondPerSecond(SliderConfig.Config.default_slider_accel * SliderConfig.Config.pan_steps_per_degree);
                stepper_pan.setTargetPositionInSteps(fEndPos_Rotation);

                CameraSlider_SetState(SLIDER_MOVING_TO_END);
            }
        break;

        case SLIDER_MOVING_TO_END:
            // Moving to end position
            if((!stepper_slide.motionComplete()) || (!stepper_pan.motionComplete()))
            {
                stepper_slide.processMovement();
                stepper_pan.processMovement();
            }
            // Move completed
            else
            {
                CameraSlider_SetState(SLIDER_READY);
            }
            break;

        case SLIDER_HOMING:
            CameraSlider_HomeSlidingRail();
        break;

        case SLIDER_READY:
        break;

        case SLIDER_WORKING:
            if((!stepper_slide.motionComplete()) || (!stepper_pan.motionComplete()))
            {
                stepper_slide.processMovement();
                stepper_pan.processMovement();
            }
        break;

        case SLIDER_STEP_FINISHED:
            ProcessStepping();
        break;

        case SLIDER_STEPPING:
            if((!stepper_slide.motionComplete()))
            {
                stepper_slide.processMovement();
            }
            else
            {
                sliderState = SLIDER_STEP_FINISHED;
            }
        break;

        default:
        return;
    }
}

void setupMotors()
{
    EnableEndstopInterrupt();

    // Connect to motors
    stepper_slide.connectToPins(PIN_MOTOR_X_STEP, PIN_MOTOR_X_DIR);
    stepper_pan.connectToPins(PIN_MOTOR_Z_STEP, PIN_MOTOR_Z_DIR);

    // Configure sliding motor
    Serial.println("Slider motor");
    Serial.print("-- Steps per mm: ");
    Serial.println(SliderConfig.Config.slide_steps_per_mm, DEC);

    stepper_slide.setStepsPerMillimeter(SliderConfig.Config.slide_steps_per_mm);
    stepper_slide.setSpeedInMillimetersPerSecond(SliderConfig.Config.default_slider_speed * SliderConfig.Config.slide_steps_per_mm);
    stepper_slide.setAccelerationInMillimetersPerSecondPerSecond(SliderConfig.Config.default_slider_accel * SliderConfig.Config.slide_steps_per_mm);

    // Configure pan motor
    // -- Revolution in steps
    stepper_pan.setSpeedInStepsPerSecond(SliderConfig.Config.pan_steps_per_degree);
    stepper_pan.setAccelerationInStepsPerSecondPerSecond(SliderConfig.Config.default_slider_accel * SliderConfig.Config.pan_steps_per_degree);
}

void CameraSlider_MoveToPositionRelative(float xPos, float xSpeed, float xAccel, float rAngle, float rSpeed, float rAccel)
{
    // Invert slider or pan motor if necessary
    xPos = SliderConfig.Config.slider_direction * xPos;
    rAngle = SliderConfig.Config.rotate_direction * rAngle;

    // Setup slider
    stepper_slide.setTargetPositionInMillimeters(xPos);
    stepper_slide.setSpeedInMillimetersPerSecond(xSpeed);
    stepper_slide.setAccelerationInMillimetersPerSecondPerSecond(xAccel);

    // Setup pan
    stepper_pan.setTargetPositionRelativeInSteps(rAngle * SliderConfig.Config.pan_steps_per_degree);
    stepper_pan.setSpeedInStepsPerSecond(rSpeed * SliderConfig.Config.pan_steps_per_degree);
    stepper_pan.setAccelerationInStepsPerSecondPerSecond(rAccel * SliderConfig.Config.pan_steps_per_degree);

    // Updatestate machine
    sliderState = SLIDER_WORKING;
}

void CameraSlider_MoveToPositionAbsolute(float xPos, float xSpeed, float xAccel, float rSteps, float rSpeed, float rAccel)
{
    // Invert slider or pan motor if necessary
    xPos = SliderConfig.Config.slider_direction * xPos;
    rSteps = SliderConfig.Config.rotate_direction * rSteps;

    // Setup slider
    stepper_slide.setTargetPositionInMillimeters(xPos);
    stepper_slide.setSpeedInMillimetersPerSecond(xSpeed);
    stepper_slide.setAccelerationInMillimetersPerSecondPerSecond(xAccel);

    // Setup pan
    stepper_pan.setTargetPositionInSteps(rSteps);
    stepper_pan.setSpeedInStepsPerSecond(rSpeed * SliderConfig.Config.pan_steps_per_degree);
    stepper_pan.setAccelerationInStepsPerSecondPerSecond(rAccel * SliderConfig.Config.pan_steps_per_degree);

    // Updatestate machine
    sliderState = SLIDER_WORKING;
}

// Copy of CameraSlider_MoveToPositionAbsolute without changing sliderState, refactor sometime
void CameraSlider_MoveToPositionAbsoluteStep(float xPos, float xSpeed, float xAccel)
{
    // Invert slider or pan motor if necessary
    xPos = SliderConfig.Config.slider_direction * xPos;

    // Setup slider
    stepper_slide.setTargetPositionInMillimeters(xPos);
    stepper_slide.setSpeedInMillimetersPerSecond(xSpeed);
    stepper_slide.setAccelerationInMillimetersPerSecondPerSecond(xAccel);
}

void CameraSlider_StartStepping()
{
    Serial.println("Start Stepping");

    float dist = 550;
    stepDist = 2;

    // floor to int
    currentStep = 0;
    maxSteps = floor(dist/stepDist);
    startPos = abs(getSliderPos());

    CameraSlider_EnableMotors(true);
    ProcessStepping();
}

void ProcessStepping()
{
    currentStep += 1;

    if(currentStep > maxSteps)
    {
        sliderState = SLIDER_IDLE;

        currentStep = 0;
        maxSteps = 0;

        CameraSlider_EnableMotors(false);
        
        Serial.println("Reached final step, stopped stepping");

        return;
    }
    
    delay(600);
    CameraControl_ReleaseShutter();
    delay(500);

    // getSliderPos seems to be unreliable, counting the position myself
    //float startPos = getSliderPos();

    float nextPos = startPos + (currentStep * stepDist);
    CameraSlider_MoveToPositionAbsoluteStep(nextPos, 4, 20);

    sliderState = SLIDER_STEPPING;
    
    Serial.print("Started step ");
    Serial.print(currentStep);
    Serial.print(" / ");
    Serial.print(maxSteps); 
    Serial.print(", Target Pos: "); 
    Serial.println(nextPos);
}

void CameraSlider_MoveToStart(float xSpeed, float xAccel, float rSpeed, float rAccel)
{
    CameraSlider_MoveToPositionAbsolute(fStartPos_Slider, xSpeed, xAccel, fStartPos_Rotation, rSpeed, rAccel);
}

void CameraSlider_MoveToEnd(float xSpeed, float xAccel, float rSpeed, float rAccel)
{
    CameraSlider_MoveToPositionAbsolute(fEndPos_Slider, xSpeed, xAccel, fEndPos_Rotation, rSpeed, rAccel);
}

void CameraSlider_HomeSlidingRail(void)
{
    Serial.println("Homing...");

    DisableEndstopInterrupt();

    CameraSlider_EnableMotors(true);

    stepper_slide.setSpeedInMillimetersPerSecond(SliderConfig.Config.homing_speed_slide);
    stepper_slide.setAccelerationInMillimetersPerSecondPerSecond(SliderConfig.Config.default_slider_accel);

    Serial.println("Homing linear rail");
    Serial.print("Dir: ");
    Serial.println(SliderConfig.Config.homing_direction, DEC);
    Serial.print("EndSW: ");
    Serial.println(PIN_END_SWICH_X_LEFT, DEC);

    if(stepper_slide.moveToHomeInMillimeters(SliderConfig.Config.homing_direction, SliderConfig.Config.homing_speed_slide, SliderConfig.Config.rail_length, PIN_END_SWICH_X_LEFT, LOW) != true)
    {
        //
        // this code is executed only if homing fails because it has moved farther
        // than maxHomingDistanceInMM and never finds the limit switch, blink the
        // LED fast forever indicating a problem
        //
        Serial.println("Failed homing!!!");

        CameraSlider_EnableMotors(false);

        while(true)
        {
        	digitalWrite(PIN_LED, HIGH);
        	delay(200);
        	digitalWrite(PIN_LED, LOW);
        	delay(200);
        }
    }

    // Move 1mm away from endstop to avoid falsely triggering it again due to bouncing when starting next move
    stepper_slide.setSpeedInMillimetersPerSecond(SliderConfig.Config.homing_speed_slide);
    stepper_slide.setAccelerationInMillimetersPerSecondPerSecond(SliderConfig.Config.default_slider_accel);
    stepper_slide.setTargetPositionInMillimeters(-2.0);
    while(!stepper_slide.processMovement()) { }

    // Reset homed position to 0
    stepper_slide.setCurrentPositionInMillimeters(0.0);

    sliderState = SLIDER_READY;
    bhomingComplete = true;

    CameraSlider_EnableMotors(false);

    Serial.println("done.");
    
    EnableEndstopInterrupt();
}


bool CameraSlider_getMotorState()
{
  return bmotorState;
}


bool CameraSlider_SetState(sliderState_t newState)
{
    if(newState <= SLIDER_FIRST)
    {
        Serial.println("Invalid state requested! (state <= SLIDER_FIRST)");
        return false;
    }
    else if( newState >= SLIDER_LAST )
    {
        Serial.println("Invalid state requested! (state >= SLIDER_LAST)");
        return false;
    }
    else
    {
        sliderState = newState;
        return true;
    }

    return false;
}


void CameraSlider_EnableMotors(bool enable)
{
    if(enable == true)
    {
        digitalWrite(PIN_MTR_nEN, LOW);
        bmotorState = true;
        CameraSlider_SetState(SLIDER_IDLE);
    }
    else
    {
        digitalWrite(PIN_MTR_nEN, HIGH);
        bmotorState = false;
        CameraSlider_SetState(SLIDER_MOTORS_OFF);
    }
}


bool CameraSlider_FormatJSON_CameraSliderStatus(char *buff, int size)
{
    int len;
    len = snprintf(buff, size, "{\"homed\":%d,\"motors\":%d,\"state\":%d,\"posX\":%f,\"posZ\":%f,\"spX\":%f,\"spZ\":%f,\"epX\":%f,\"epZ\":%f}",
                 bhomingComplete,
                 bmotorState,
                 sliderState,
                 getSliderPos(),
                 getRotationPos(true),
                 fStartPos_Slider,
                 SliderConfig.Config.rotate_direction*(fStartPos_Rotation/SliderConfig.Config.pan_steps_per_degree),
                 fEndPos_Slider,
                 SliderConfig.Config.rotate_direction*(fEndPos_Rotation/SliderConfig.Config.pan_steps_per_degree)

                );

    if(len > 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool CameraSlider_FormatJSON_CameraConfig(char *buff, int size)
{
    int len;
    len = snprintf(buff, size, "{\"rail_length\":%d,\"dir_homing\":%d,\"dir_slider\":%d,\"dir_rotation\":%d,\"slider_steps_per_mm\":%d,\"rotation_steps_per_deg\":%d,\"homing_speed_slider\":%d,\"homing_speed_rotation\":%d}",
                SliderConfig.Config.rail_length,
                SliderConfig.Config.homing_direction,
                SliderConfig.Config.slider_direction,
                SliderConfig.Config.rotate_direction,
                SliderConfig.Config.slide_steps_per_mm,
                SliderConfig.Config.pan_steps_per_degree,
                SliderConfig.Config.homing_speed_slide,
                SliderConfig.Config.homing_speed_pan
            );

    if(len > 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool CameraSlider_getHomingState(void)
{
  return bhomingComplete;
}


bool CameraSlider_StartMotion(void)
{
    // Configure slider
    stepper_slide.setTargetPositionInMillimeters(fStartPos_Slider);
    stepper_slide.setSpeedInMillimetersPerSecond(SliderConfig.Config.default_slider_speed);
    stepper_slide.setAccelerationInMillimetersPerSecondPerSecond(SliderConfig.Config.default_slider_accel);

    // Configure pan
    stepper_pan.setTargetPositionInSteps(fStartPos_Rotation);
    stepper_pan.setSpeedInStepsPerSecond(SliderConfig.Config.default_rotate_speed * SliderConfig.Config.pan_steps_per_degree);
    stepper_pan.setAccelerationInStepsPerSecondPerSecond(SliderConfig.Config.default_rotate_accel * SliderConfig.Config.pan_steps_per_degree);

    CameraSlider_SetState(SLIDER_MOVING_TO_START);

    return true;
}

bool CameraSlider_SetDuration(uint32_t durationSec)
{
    slideDurationSec = durationSec;
    return true;
}

bool CameraSlider_SetStartPosition(float slideStartPos, float rotStartPos)
{
    fStartPos_Slider = slideStartPos;
    fStartPos_Rotation = rotStartPos;

    return true;
}


bool CameraSlider_SetEndPosition(float slideEndPos, float rotEndPos)
{
    fEndPos_Slider = slideEndPos;
    fEndPos_Rotation = rotEndPos;

    return true;
}

bool CameraSlider_StoreAsStartPosition(void)
{
    fStartPos_Slider = getSliderPos();
    fStartPos_Rotation = getRotationPos(false);

    return true;
}


bool CameraSlider_StoreAsEndPosition(void)
{
    fEndPos_Slider = getSliderPos();
    fEndPos_Rotation = getRotationPos(false);

    return true;
}

bool CameraSlider_StoreAsRotationHome(void)
{
    stepper_pan.setTargetPositionInSteps(0);
    stepper_pan.setCurrentPositionInMillimeters(0);
    stepper_pan.setCurrentPositionInRevolutions(0);
    stepper_pan.setCurrentPositionInSteps(0);

    return true;
}

float getSliderPos()
{
    return stepper_slide.getCurrentPositionInMillimeters();
}

float getRotationPos(bool calculateDegrees)
{
    #if 0
    return stepper_pan.getCurrentPositionInRevolutions();
    #else
    if(calculateDegrees)
    {
        return SliderConfig.Config.rotate_direction*(stepper_pan.getCurrentPositionInSteps()/SliderConfig.Config.pan_steps_per_degree);
    }
    else
    {
        return SliderConfig.Config.rotate_direction*stepper_pan.getCurrentPositionInSteps();
    }
    #endif
}

void CameraSlider_UpdateRailLength(uint32_t rail_length)
{
    SliderConfig.Config.rail_length = rail_length;
    SliderConfig.Write();
}

void EnableEndstopInterrupt()
{
    attachInterrupt(digitalPinToInterrupt(PIN_END_SWICH_X_LEFT), endstopISR_Left, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_END_SWICH_X_RIGHT), endstopISR_Right, RISING);
}

void DisableEndstopInterrupt()
{
    detachInterrupt(digitalPinToInterrupt(PIN_END_SWICH_X_LEFT));
    detachInterrupt(digitalPinToInterrupt(PIN_END_SWICH_X_RIGHT));
}

void endstopISR_Left()
{
    DisableEndstopInterrupt();

    CameraSlider_EnableMotors(false);
    Serial.println("Left Endstop triggered! Stopped motors.");

    EnableEndstopInterrupt();
}

void endstopISR_Right()
{
    DisableEndstopInterrupt();

    CameraSlider_EnableMotors(false);
    Serial.println("Right Endstop triggered! Stopped motors.");

    EnableEndstopInterrupt();
}