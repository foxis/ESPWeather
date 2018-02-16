#ifndef MY_CONFIG_H
#define MY_CONFIG_H


#include <SPI.h>
#include <Wire.h>


#include "ConfigBase.h"
#include "Telemetry.h"
#if defined(ESP_WEATHER_VARIANT_OLED)
#include "DisplayOLED.h"
#elif defined(ESP_WEATHER_VARIANT_EPAPER)
#include "DisplayEPAPER.h"
#else
#include "DisplayHEADLESS.h"
#endif
#include "MQTT.h"



class Configuration : ConfigurationBase
{
public:
	MQTT mqtt;
	Display _display;
	Telemetry _telemetry;

	Configuration():
	  ConfigurationBase(_display, _telemetry),
		mqtt(*this),
		_display(*this),
		_telemetry(*this)
	{

	}

	void begin() {
		SPIFFS.begin();
		telemetry.begin();
		display.begin();

		display.publish_status("Initializing");

		if (!loadConfig())
		{
			// Default config
			OTA.addAP(WIFI_SSID, WIFI_PASSWORD);
			OTA.addAP(WIFI_SSID1, WIFI_PASSWORD1);
		}

		OTA.allowOpen(true);

		OTA.onConnect([](const String& ssid, EasyOTA::STATE state){
			if (ConfigurationBase::instance->myName == "") {
				ConfigurationBase::instance->setMyName(WiFi.macAddress());
			}
			ConfigurationBase::instance->display.publish_status("WIFI: " + ConfigurationBase::instance->OTA.currentAP());
		});

		mqtt.begin();

		last_m = millis();
}

	void loop(long now)
	{
		OTA.loop(now);
		mqtt.loop(now);
		telemetry.loop(now);

		// Sleep after so much seconds
		if (can_sleep && now - last_m > 30 * 1000)	{
			deepsleep();
		}
	}

	virtual void deepsleep()
	{
		mqtt.disconnect();
		ConfigurationBase::deepsleep();
	}

	virtual void restart()
	{
		mqtt.disconnect();
		ConfigurationBase::restart();
	}

private:
	long last_m;
};

#endif
