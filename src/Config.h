/**
*  Copyright (C) 2018  foxis (Andrius Mikonis <andrius.mikonis@gmail.com>)
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **/
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


class Configuration : public ConfigurationBase
{
public:
	MQTT mqtt;
	Display _display;
	Telemetry _telemetry;
	AsyncWebServer server;

	Configuration():
	  ConfigurationBase(_display, _telemetry),
		mqtt(*this),
		_display(*this),
		_telemetry(*this),
		server(80)
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
#if defined(WIFI_SSID1)
			OTA.addAP(WIFI_SSID1, WIFI_PASSWORD1);
#endif
#if defined(WIFI_SSID2)
			OTA.addAP(WIFI_SSID2, WIFI_PASSWORD2);
#endif
		}

		OTA.allowOpen(true);

		OTA.onConnect([](const String& ssid, EasyOTA::STATE state){
			if (ConfigurationBase::instance->myName == "") {
				ConfigurationBase::instance->setMyName(WiFi.macAddress());
			}
			ConfigurationBase::instance->display.publish_status("WIFI: " + ConfigurationBase::instance->OTA.currentAP());
		});

		mqtt.begin();

		server.serveStatic("/", SPIFFS, "/web").setDefaultFile("index.html");
		server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
			request->send(200, "text/plain", String(ESP.getFreeHeap()));
		});
		server.on("/keepalive", HTTP_GET, [](AsyncWebServerRequest *request) {
			request->send(200, "text/plain", String("OK"));
			ConfigurationBase::instance->keepalive();
		});
		server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
			AsyncWebServerResponse *response = request->beginResponse(SPIFFS, CONFIG_FILE);
			//request->send(SPIFFS, "/profiles.json");
			response->addHeader("Access-Control-Allow-Origin", "*");
			response->addHeader("Access-Control-Allow-Methods", "GET");
			response->addHeader("Content-Type", "application/json");
			request->send(response);
		});
		server.on("/config", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
			ConfigurationBase::instance->save_file(request, CONFIG_FILE, data, len, index, total);
		});
		server.onNotFound([](AsyncWebServerRequest *request) { request->send(404); });
		server.begin();

		last_m = millis();
}

	void loop(unsigned long now)
	{
		OTA.loop(now);
		mqtt.loop(now);
		telemetry.loop(now);
		display.loop(now);

		// Sleep after so much seconds
		if (can_sleep && now - last_m > this->timeout || this->maxreadings && (mqtt.readings >= this->maxreadings))	{
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
};

#endif
