#include "include/PersistSettings.h"

#ifndef __CAMERASLIDER_CFG__
#define __CAMERASLIDER_CFG__

// Configure oTA
#define MDNS_NAME       "sk_camera_slider"
#define OTA_NAME        "sk_camera_slider"
#define OTA_PASSWORD    "change_this_please"

// Various GPIO connections to control motor direction
// enable/disbale, reset, end/limit switch etc.
#define PIN_MOTOR_X_STEP			19
#define PIN_MOTOR_X_DIR             18
#define PIN_MOTOR_Z_STEP            17
#define PIN_MOTOR_Z_DIR             16
#define PIN_MTR_nRST                5
#define PIN_MTR_nEN                 4
#define PIN_LED 					2
#define PIN_END_SWICH_X_LEFT		34
#define PIN_END_SWICH_X_RIGHT	    32
#define PIN_FOCUS   			    22
#define PIN_SHUTTER   			    23


// "Mechanical" configuration of the camera slider
#define RAIL_LENGTH_MM			    330       // Rail (2020 extrusion) length that platform can slide along (in milimeters)
#define SLIDE_STEPS_PER_MM          187
#define MIN_STEP_SLIDER             5         // Minimum allowed step for sliding platform
#define PAN_STEPS_PER_DEGREE        78


// Positioning and speed default values
// In normal operation, these values will get overwritten by HTTP request
#define DEFAULT_SLIDE_TO_POS_SPEED  30.0
#define DEFAULT_SLIDE_TO_POS_ACCEL  60.0
#define DEFAULT_ROTATE_TO_POS_SPEED 30.0
#define DEFAULT_ROTATE_TO_POS_ACCEL 60.0


#define DEFAULT_HOMING_SPEED_SLIDE  DEFAULT_SLIDE_TO_POS_SPEED
#define DEFAULT_HOMING_SPEED_PAN    PAN_STEPS_PER_DEGREE


// Homing settings

typedef enum 
{ 
	SLIDER_FIRST = 0,
    SLIDER_MOTORS_OFF, 
    SLIDER_IDLE, 
    SLIDER_HOMING,
    SLIDER_MOVING_TO_START,
    SLIDER_MOVING_TO_END,
    SLIDER_READY,
    SLIDER_WORKING,    
    SLIDER_STEPPING,
    SLIDER_STEP_FINISHED,
    SLIDER_LAST
} sliderState_t;

extern const char* sliderStateStr[];

typedef enum
{
    HOMING_DIRECTION = 0,
    SLIDING_DIRECTION,
    PAN_DIRECTION,
    SLIDER_STEPS_PER_MM,
    ROTATION_STEPS_PER_DEG,
    HOMING_SPEED_SLIDE,
    HOMING_SPEED_PAN,
    MIN_SLIDER_STEP
} CameraSliderConfig_t;


typedef enum
{
    MOVE_RELATIVE = 0,
    MOVE_ABSOLUTE,
    MOVE_TO_STORED_POSITION_START,
    MOVE_TO_STORED_POSITION_END
} CameraSliderMovement_t;

typedef enum 
{ 
    CAMERA_IDLE = 0,
    CAMERA_SHUTTER_FOCUS,    
    CAMERA_SHUTTER_FOCUSING,
	CAMERA_SHUTTER_RELEASE,
	CAMERA_SHUTTER_RELEASING
} CameraState_t;




// We will use persistent storage to store our Camera Slider configuration
// but also allow us to edit it trough web/api
// This way we can decouple firmware from electronics/mech changes
// i.e We can change rail lenght or motor direction without recompiling firmware
// Note: You should not edit config below. Instead modify defaults inside `config_cameraslider.h`
struct SliderConfigStruct
{
    static const unsigned int Version = 1;

    uint16_t rail_length = RAIL_LENGTH_MM;
    uint16_t min_slider_step = MIN_STEP_SLIDER;


    int homing_direction = -1;  // Specify if we should reverse homing direction
    int slider_direction = 1;   // Increase (1) or decrease(-1) steps to get positive movement
    int rotate_direction = 1;   // Increase (1) or decrease(-1) steps to get positive movement

    uint16_t slide_steps_per_mm = SLIDE_STEPS_PER_MM;
    uint16_t pan_steps_per_degree = PAN_STEPS_PER_DEGREE;

    uint16_t homing_speed_slide = DEFAULT_HOMING_SPEED_SLIDE;
    uint16_t homing_speed_pan   = DEFAULT_HOMING_SPEED_PAN;

    float default_slider_speed = DEFAULT_SLIDE_TO_POS_SPEED;
    float default_slider_accel = DEFAULT_SLIDE_TO_POS_ACCEL;
    float default_rotate_speed = DEFAULT_ROTATE_TO_POS_SPEED;
    float default_rotate_accel = DEFAULT_ROTATE_TO_POS_ACCEL;
};

extern PersistSettings<SliderConfigStruct> SliderConfig;

#endif
