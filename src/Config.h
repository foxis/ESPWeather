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

#include "ConfigBase.h"
#include "Telemetry.h"
#if defined(ESP_WEATHER_VARIANT_OLED)
#include "DisplayOLED.h"
#elif defined(ESP_WEATHER_VARIANT_EPAPER)
#include "DisplayEPAPER.h"
#elif defined(ESP_WEATHER_VARIANT_PRO)
#include "DisplaySerial.h"
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
		loadConfig();

		Wire.begin(SDA0, SCL0);
#if defined(ESP_WEATHER_VARIANT_PRO)
		pinMode(POWER_PIN, OUTPUT);
		digitalWrite(POWER_PIN, LOW);
		Serial.begin(74880);
#endif

		SERIAL_LN("Power on, initializing...");

		telemetry.begin();
		display.begin();

		display.publish_status("Initializing");

		OTA.allowOpen(this->allowopen);
		if (this->is_static) {
			OTA.setStaticIP(static_ip, static_dns, static_gateway, static_subnet);
		}

		OTA.onConnect([](const String& ssid, EasyOTA::STATE state){
			if (ConfigurationBase::instance->myName == "") {
				ConfigurationBase::instance->setMyName(WiFi.macAddress());
			}
			ConfigurationBase::instance->display.publish_status("WIFI: " + ConfigurationBase::instance->OTA.currentAP());
			SERIAL_V("Connected to AP: ");
			SERIAL_LN(ConfigurationBase::instance->OTA.currentAP());
		});

		mqtt.begin();

		if (!woke_up) {
			SERIAL_LN("Entering WebUI config mode...");
			ConfigurationBase::instance->telemetry.build_json = true;

			server.serveStatic("/", SPIFFS, "/web").setDefaultFile("index.html");
			server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
				request->send(200, "text/plain", String(ESP.getFreeHeap()));
			});
			server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
				request->send(200, "text/plain", String("OK"));
				ConfigurationBase::instance->restart();
			});
			server.on("/info", HTTP_GET, [](AsyncWebServerRequest *request) {
				ConfigurationBase::instance->keepalive();
				StaticJsonBuffer<1024> buf;
				JsonObject &obj = buf.createObject();
				obj["Build"] = __DATE__;
				obj["Version"] = ESP_WEATHER_VERSION;
				obj["Hostname"] = ESP_WEATHER_HOSTNAME;
				obj["MAC"] = WiFi.macAddress();
				obj["Heap"] = ESP.getFreeHeap();
				String info;
				obj.printTo(info);

				request->send(200, "application/json", info);
			});
			server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request) {
				ConfigurationBase::instance->keepalive();
				ConfigurationBase::instance->telemetry.lock();
				request->send(200, "application/json", ConfigurationBase::instance->telemetry.json);
				ConfigurationBase::instance->telemetry.unlock();
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
		if (wifi_enabled || !woke_up)
			OTA.loop(now);
		mqtt.loop(now);
		telemetry.loop(now);
		display.loop(now);

		unsigned long timeout = woke_up ? this->timeout : 5 * 60 * 1000;
		// Sleep after so much seconds (timeout is in milliseconds)
		if (can_sleep && (now > last_m && now - last_m > timeout) || this->maxreadings && (mqtt.readings >= this->maxreadings))	{
			deepsleep();
		}
	}

	virtual void deepsleep()
	{
		mqtt.disconnect();
		display.end();
		#if defined(ESP_WEATHER_VARIANT_PRO)
		digitalWrite(POWER_PIN, HIGH);
		#endif
		ESP.deepSleep(woke_up ? this->deepsleeptimeout : 1000000);
	}

	virtual void restart()
	{
		mqtt.disconnect();
		ConfigurationBase::restart();
	}
};

#endif
