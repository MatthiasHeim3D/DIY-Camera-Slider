/*
CameraSlider - Web
Description: This file contains all the functions we use for receiving and responding to HTTP requests
*/

#include "version.h"
#include <ESPAsyncWebServer.h>
#include "SliderConfig.h"

extern AsyncWebServer server;

String template_const_processor(const String& var);
void setupWebServer(void);
void WebAPI_MoveToPosition(CameraSliderMovement_t move_type, AsyncWebServerRequest *request);
bool WebAPI_GetIntValueFromRequest(AsyncWebServerRequest *pRequest, const char *argName, int32_t *pInt);
bool WebAPI_UpdateMotorConfig(CameraSliderConfig_t parameter, AsyncWebServerRequest *pRequest);