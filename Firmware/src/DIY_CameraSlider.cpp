#include <Arduino.h>
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include <ESPmDNS.h>
#include <FlexyStepper.h>
#include "config_wifi.h"
#include "include/PersistSettings.h"
#include "DIY_CameraSlider_MotorControl.h"
#include "DIY_CameraSlider_CameraControl.h"
#include "SliderConfig.h"
#include "DIY_CameraSlider_Web.h"

// Peristent device config
PersistSettings<SliderConfigStruct> SliderConfig(SliderConfigStruct::Version);

AsyncWebServer server(80);
int WiFi_status = WL_IDLE_STATUS; 

void setup()
{
    // Configure Serial communication
	Serial.begin(115200);
    Serial.println("DIY Camera Slider");
    
    SliderConfig.Begin();
    if( SliderConfig.Valid() )
    {
        Serial.println("Reloading camera slider settings.");
    }
    else
    {
        Serial.println("Camera settings invalid. Resetting to default.");
    }

    // Configure and initialize GPIOs
	pinMode(PIN_LED, OUTPUT);
	pinMode(PIN_MTR_nRST, OUTPUT);
	pinMode(PIN_MTR_nEN, OUTPUT);
	pinMode(PIN_SHUTTER, OUTPUT);
	pinMode(PIN_END_SWICH_X_LEFT, INPUT_PULLUP);
	pinMode(PIN_END_SWICH_X_RIGHT, INPUT_PULLUP);
	digitalWrite(PIN_LED, HIGH);
	digitalWrite(PIN_MTR_nRST, HIGH);
	digitalWrite(PIN_MTR_nEN, HIGH);
	digitalWrite(PIN_SHUTTER, LOW);

	delay(1000);

	// Connect to WiFi
	while ( WiFi_status != WL_CONNECTED)
    {
		Serial.print("Connecting to SSID: ");
		Serial.println(ssid);
		WiFi_status = WiFi.begin(ssid, password);

		// wait 5 seconds before retrying
		delay(5000);
	}

	// Initialize SPIFFS
	if(!SPIFFS.begin(true))
    {
		Serial.println("An Error has occurred while mounting SPIFFS");
		while(1);
	}

	Serial.print("WiFi IP: ");
	Serial.println(WiFi.localIP());

    if (!MDNS.begin(MDNS_NAME))
    {
        Serial.println("Error starting mDNS");
    }
    else
    {
        Serial.println((String) "mDNS http://" + MDNS_NAME + ".local");
    }

	// Setup motors
	setupMotors();
	CameraSlider_EnableMotors(true);
	digitalWrite(PIN_LED, LOW);

    // Initialize Web server
	setupWebServer();
	server.begin();

    // Debug message to signal we are initialized and entering loop
	Serial.println("Ready to go.");
}


void loop()
{
	while(1)
	{
		CameraSlider_tick();
        CameraControl_tick();
	}
}
