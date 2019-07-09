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
#ifndef MY_CONFIG_BASE_H
#define MY_CONFIG_BASE_H

#if defined(ESP_WEATHER_VARIANT_EPAPER)
#define ARDUINO_HOSTNAME "ESPWeather-epaper"
#elif defined(ESP_WEATHER_VARIANT_OLED)
#define ARDUINO_HOSTNAME "ESPWeather-oled"
#elif defined(ESP_WEATHER_VARIANT_PRO)
#define ARDUINO_HOSTNAME "ESPWeather-Pro"
#else
// ESP_WEATHER_VARIANT_HEADLESS
#define ARDUINO_HOSTNAME "ESPWeather"
#endif

#if defined(ESP_WEATHER_VARIANT_UI)
#undef ARDUINO_HOSTNAME
#define ARDUINO_HOSTNAME "ESPWeather-UI"
#endif

// Config parameters definitions
#define CONFIG_FILE 					"/config.json"
#define MY_NAME					 			"serverName"
#define WIFI_ENABLED 					"wifi"
#define NETWORKS 							"networks"
#define DEEPSLEEPTIMEOUT 			"deepsleeptimeout"
#define TIMEOUT 							"timeout"
#define CANSLEEP 							"cansleep"
#define MAXREADINGS 					"maxreadings"
#define ALLOWOPEN 						"allowopen"
#define MQTT_USER 						"mqtt_user"
#define MQTT_PASSWORD 				"mqtt_password"
#define MQTT_PORT 						"mqtt_port"
#define MQTT_URL 							"mqtt_url"
#define NTP_URL 							"ntp_url"
#define MQTT_LISTEN_NAMES 		"mqtt_listen_names"

#define STATIC 								"static"
#define STATIC_IP 						"static_ip"
#define STATIC_GATEWAY 				"static_gateway"
#define STATIC_SUBNET 				"static_subnet"

// PIN definitions
#define DIVIDER_SELECTOR 			5
#define SENSOR_SWITCH					2

#if defined(ESP_WEATHER_VARIANT_PRO)
	#define SDA0 								4
	#define SCL0 								5
	#define SDA1  							13
	#define SCL1  							12
	#define SERIAL_RX 					3
	#define SERIAL_TX 					1
	#define ONE_WIRE_BUS				14
	#define POWER_PIN						0
#else
	#define SDA0 								1
	#define SCL0 								3
#endif
// 3 RX
// 1 TX

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "DisplayBase.h"
#include "TelemetryBase.h"

#include <EasyOTA.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>
#include <ArduinoJson.h>
#if defined(ESP32)
#include "SPIFFS.h"
#else
#include <FS.h>
#endif

#if defined(ESP_WEATHER_VARIANT_PRO)
#define SERIAL_LN(a) Serial.println(a)
#define SERIAL_V(a) Serial.print(a)
#else
#define SERIAL_LN(a)
#define SERIAL_V(a)
#endif


class ConfigurationBase
{
public:
	EasyOTA OTA;
	DisplayBase& display;
	TelemetryBase& telemetry;
	static ConfigurationBase* instance;

	// Network setup
	bool wifi_enabled;
	String mqtt_user;
	String mqtt_password;
	String mqtt_url;
	int mqtt_port;
	std::vector<String> mqtt_listen_names;

	String ntp_url;

	bool allowopen;

	bool is_static;
	IPAddress static_ip;
	IPAddress static_gateway;
	IPAddress static_subnet;

	// Station name
	String myName;

	// Power management
	bool can_sleep;
	unsigned long timeout;
	unsigned long deepsleeptimeout;
	int maxreadings;
	bool woke_up;

	// timekeeping
	unsigned long last_m;


	ConfigurationBase(DisplayBase& display, TelemetryBase& telemetry) :
		OTA(ARDUINO_HOSTNAME),
		display(display),
		telemetry(telemetry)
	{
		instance = this;
		myName = "";
		can_sleep = true;
		timeout = 15000;
		deepsleeptimeout = 1800000000;
		maxreadings = 3;
		allowopen = false;
		wifi_enabled = true;
		is_static = false;
		woke_up = ESP.getResetInfoPtr()->reason != REASON_DEFAULT_RST &&
							ESP.getResetInfoPtr()->reason != REASON_SOFT_RESTART &&
							ESP.getResetInfoPtr()->reason != REASON_EXT_SYS_RST;
	}

	void setMyName(const String& name)
	{
		myName = name;
		saveConfig();
		display.publish_name(myName);
	}

	void addAP(const char * ssid, const char * passw)
	{
		OTA.addAP(ssid, passw);
		saveConfig();
	}
	void removeAP(const char * ssid)
	{
		OTA.removeAP(ssid);
		saveConfig();
	}

	bool loadConfig() {
		File configFile = SPIFFS.open(CONFIG_FILE, "r");
	  if (!configFile) {
	    return false;
	  }

	  size_t size = configFile.size();
	  if (size > 1024) {
			configFile.close();
	    return false;
	  }

		StaticJsonBuffer<1024> jsonBuffer;
		JsonObject &json = jsonBuffer.parseObject(configFile);

		if (!json.success()) {
			configFile.close();
	    return false;
	  }

		if (json.containsKey(MY_NAME))
	  	myName = json[MY_NAME].as<String>();
		else
		{
			configFile.close();
			return false;
		}

		if (json.containsKey(WIFI_ENABLED))
			wifi_enabled = json[WIFI_ENABLED];
		if (json.containsKey(STATIC))
			is_static = json[STATIC];
		if (json.containsKey(STATIC_IP))
			static_ip.fromString(json[STATIC_IP].as<String>());
		if (json.containsKey(STATIC_GATEWAY))
			static_gateway.fromString(json[STATIC_GATEWAY].as<String>());
		if (json.containsKey(STATIC_SUBNET))
			static_subnet.fromString(json[STATIC_SUBNET].as<String>());

		if (json.containsKey(DEEPSLEEPTIMEOUT))
			deepsleeptimeout = json[DEEPSLEEPTIMEOUT];
		if (json.containsKey(TIMEOUT))
			timeout = json[TIMEOUT];
		if (json.containsKey(CANSLEEP))
			can_sleep = json[CANSLEEP];
		if (json.containsKey(MAXREADINGS))
			maxreadings = json[MAXREADINGS];
		if (json.containsKey(ALLOWOPEN))
			allowopen = json[ALLOWOPEN];

		if (json.containsKey(MQTT_USER))
			mqtt_user = json[MQTT_USER].as<String>();
		if (json.containsKey(MQTT_PASSWORD))
			mqtt_password = json[MQTT_PASSWORD].as<String>();
		if (json.containsKey(MQTT_PORT))
			mqtt_port = json[MQTT_PORT];
		if (json.containsKey(MQTT_URL))
			mqtt_url = json[MQTT_URL].as<String>();
		if (json.containsKey(NTP_URL))
			ntp_url = json[NTP_URL].as<String>();
		if (json.containsKey(MQTT_LISTEN_NAMES)) {
			JsonArray& ja = json[MQTT_LISTEN_NAMES];
			JsonArray::iterator I = ja.begin();
			while (I != ja.end()) {
				mqtt_listen_names.push_back(*I);
				++I;
			}
		}

		if (json.containsKey(NETWORKS)){
			if (woke_up) {
				JsonObject& jo = json[NETWORKS];
				JsonObject::iterator I = jo.begin();
				while (I != jo.end())
				{
					OTA.addAP(I->key, I->value);
					++I;
				}
			}
		} else
		{
			configFile.close();
			return false;
		}

		configFile.close();
	  return true;
	}

	bool saveConfig() {
		StaticJsonBuffer<1024> jsonBuffer;
		File configFile = SPIFFS.open(CONFIG_FILE, "w");
	  if (!configFile) {
			SPIFFS.format();
			configFile = SPIFFS.open(CONFIG_FILE, "w");
	    if (!configFile)
				return false;
	  }
		JsonObject& json = jsonBuffer.createObject();
		json[MY_NAME] = myName;
		json[WIFI_ENABLED] = wifi_enabled;
		json[STATIC] = is_static;
		json[STATIC_IP] = static_ip.toString();
		json[STATIC_GATEWAY] = static_gateway.toString();
		json[STATIC_SUBNET] = static_subnet.toString();
		json[DEEPSLEEPTIMEOUT] = deepsleeptimeout;
		json[TIMEOUT] = timeout;
		json[CANSLEEP] = can_sleep;
		json[MAXREADINGS] = maxreadings;
		json[ALLOWOPEN] = allowopen;
		json[MQTT_USER] = mqtt_user;
		json[MQTT_PASSWORD] = mqtt_password;
		json[MQTT_PORT] = mqtt_port;
		json[MQTT_URL] = mqtt_url;
		json[NTP_URL] = ntp_url;

		JsonArray& arr = json.createNestedArray(MQTT_LISTEN_NAMES);
		std::vector<String>::iterator I = mqtt_listen_names.begin();
		while (I != mqtt_listen_names.end()) {
			arr.add(*I);
			++I;
		}

		JsonObject& data = json.createNestedObject(NETWORKS);
		OTA.eachAP([](const String& ssid, const String& pw, void * ja){
			(*(JsonObject*)ja)[ssid] = pw;
		}, (void*)&data);

	  json.printTo(configFile);

		configFile.close();
	  return true;
	}

	bool save_file(AsyncWebServerRequest *request, const String& fname, uint8_t * data, size_t len, size_t index, size_t total)
	{
		Serial.println("Saving config " + fname +" len/index: " + String(len) + "/" +  String(index));

		File f = SPIFFS.open(fname, index != 0 ? "a" : "w");
	  if (!f) {
			request->send(404, "application/json", "{\"msg\": \"ERROR: couldn't " + fname + " file for writing!\"}");
			return false;
		}

		// TODO sanity checks

		f.write(data, len);

		if (f.size() >= total)
		{
			request->send(200, "application/json", "{\"msg\": \"INFO: " + fname + " saved!\"}");
			Serial.println("Saving config... DONE");
		}

		f.close();
		return true;
	}

	virtual void deepsleep()
	{
		display.end();
		SERIAL_LN("Going to sleep...");
		ESP.deepSleep(woke_up ? this->deepsleeptimeout : 1000000);
	}

	void keepalive() {
		last_m = millis();
	}

	virtual void restart()
	{
		display.end();
		SERIAL_LN("Restarting...");
		ESP.deepSleep(1000000);
		//ESP.restart();
	}

};

ConfigurationBase* ConfigurationBase::instance = NULL;

#endif
