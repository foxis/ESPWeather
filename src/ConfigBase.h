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

#include "wificonfig.h"
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

#define CONFIG_FILE "/config.json"
#define MY_NAME "serverName"
#define NETWORKS "networks"
#define DEEPSLEEPTIMEOUT "deepsleeptimeout"
#define TIMEOUT "timeout"
#define CANSLEEP "cansleep"
#define MAXREADINGS "maxreadings"
#define ALLOWOPEN "allowopen"
#define SDA  1
#define SCL 3
// 3 RX
// 1 TX

class ConfigurationBase
{
public:
	EasyOTA OTA;
	DisplayBase& display;
	TelemetryBase& telemetry;
	static ConfigurationBase* instance;

	String myName;
	bool can_sleep;
	unsigned long timeout;
	unsigned long deepsleeptimeout;
	int maxreadings;
	bool allowopen;

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
	#if defined(PUYA_ISSUE)
		return false;
	#endif

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
	  	myName = String((const char *)json[MY_NAME]);
		else
		{
			configFile.close();
			return false;
		}

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

		if (json.containsKey(NETWORKS)){
			JsonObject& jo = json[NETWORKS];
			JsonObject::iterator I = jo.begin();
			while (I != jo.end())
			{
				OTA.addAP(I->key, I->value);
				++I;
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
		#if defined(PUYA_ISSUE)
			return false;
		#endif

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
		json[DEEPSLEEPTIMEOUT] = deepsleeptimeout;
		json[TIMEOUT] = timeout;
		json[CANSLEEP] = can_sleep;
		json[MAXREADINGS] = maxreadings;
		json[ALLOWOPEN] = allowopen;
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
		ESP.deepSleep(this->deepsleeptimeout);
	}

	void keepalive() {
		last_m = millis();
	}

	virtual void restart()
	{
		display.end();
		ESP.restart();
	}

};

ConfigurationBase* ConfigurationBase::instance = NULL;

#endif
