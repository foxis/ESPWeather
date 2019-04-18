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

		loadConfig();

		OTA.allowOpen(this->allowopen);

		OTA.onConnect([](const String& ssid, EasyOTA::STATE state){
			if (ConfigurationBase::instance->myName == "") {
				ConfigurationBase::instance->setMyName(WiFi.macAddress());
			}
			ConfigurationBase::instance->display.publish_status("WIFI: " + ConfigurationBase::instance->OTA.currentAP());
		});

		mqtt.begin();

		if (!woke_up) {
			server.serveStatic("/", SPIFFS, "/web").setDefaultFile("index.html");
			server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
				request->send(200, "text/plain", String(ESP.getFreeHeap()));
			});
			server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
				request->send(200, "text/plain", String("OK"));
				ConfigurationBase::instance->restart();
			});
			server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request) {
				ConfigurationBase::instance->keepalive();
				StaticJsonBuffer<200> buf;
				String json;
				JsonObject &obj = buf.createObject();
				obj["temperature"] = ConfigurationBase::instance->telemetry._temperature;
				obj["humidity"] = ConfigurationBase::instance->telemetry._humidity;
				obj["pressure"] = ConfigurationBase::instance->telemetry._pressure;
				obj["light"] = ConfigurationBase::instance->telemetry._light;
				obj["battery"] = ConfigurationBase::instance->telemetry._battery;
				obj.printTo(json);
				request->send(200, "application/json", json);
			});
			server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
				AsyncWebServerResponse *response = request->beginResponse(SPIFFS, CONFIG_FILE);
				response->addHeader("Access-Control-Allow-Origin", "*");
				response->addHeader("Access-Control-Allow-Methods", "GET");
				response->addHeader("Content-Type", "application/json");
				request->send(response);
			});
			server.on("/config", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
				ConfigurationBase::instance->save_file(request, CONFIG_FILE, data, len, index, total);
				ConfigurationBase::instance->loadConfig();
			});
			server.onNotFound([](AsyncWebServerRequest *request) { request->send(404); });
			server.begin();
		}

		last_m = millis();
	}

	void loop(unsigned long now)
	{
		OTA.loop(now);
		mqtt.loop(now);
		telemetry.loop(now);
		display.loop(now);

		// Sleep after so much seconds (timeout is in milliseconds)
		unsigned int timeout = woke_up ? this->timeout : 5 * 60 * 1000;
		if (can_sleep && now - last_m > timeout || this->maxreadings && (mqtt.readings >= this->maxreadings))	{
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
