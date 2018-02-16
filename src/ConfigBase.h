#ifndef MY_CONFIG_BASE_H
#define MY_CONFIG_BASE_H

#include "wificonfig.h"
#include "DisplayBase.h"
#include "TelemetryBase.h"

#include <EasyOTA.h>

#include <ArduinoJson.h>
#if defined(ESP32)
#include "SPIFFS.h"
#else
#include <FS.h>
#endif

#define CONFIG_FILE "/config.json"
#define MY_NAME "serverName"
#define NETWORKS "networks"
#define SDA  1
#define SCL 3

class ConfigurationBase
{
public:
	EasyOTA OTA;
	DisplayBase& display;
	TelemetryBase& telemetry;
	static ConfigurationBase* instance;

	String myName;
	bool can_sleep;

	ConfigurationBase(DisplayBase& display, TelemetryBase& telemetry) :
		OTA(ARDUINO_HOSTNAME),
		display(display),
		telemetry(telemetry)
	{
		instance = this;
		myName = "";
		can_sleep = true;
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
		JsonObject& data = json.createNestedObject(NETWORKS);
		OTA.eachAP([](const String& ssid, const String& pw, void * ja){
			(*(JsonObject*)ja)[ssid] = pw;
		}, (void*)&data);

	  json.printTo(configFile);

		configFile.close();
	  return true;
	}

	virtual void deepsleep()
	{
		display.publish_status("");
		ESP.deepSleep(30L*60L*1000000L /*, optional RFMode mode*/);
	}

	virtual void restart()
	{
		display.publish_status("");
		ESP.restart();
	}

};

ConfigurationBase* ConfigurationBase::instance = NULL;

#endif
