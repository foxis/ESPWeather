#include "wificonfig.h"
#include <EasyOTA.h>
#include <SPI.h>
#include <Wire.h>
#include <ArduinoJson.h>
#if defined(ESP32)
#include "SPIFFS.h"
#else
#include <FS.h>
#endif

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_BMP280.h>

#include <SSD1306.h>
#include <PubSubClient.h>

// PUYA flash is not supported by SPIFFS library
#define PUYA_ISSUE

EasyOTA OTA(ARDUINO_HOSTNAME);
DHT_Unified dht11(02, DHT11);
Adafruit_BMP280 bme;
WiFiClient espClient;
bool _can_sleep = true;
PubSubClient client(espClient);
String myName = "";

#define SDA  1
#define SCL 3

#if defined(ESP_WEATHER_VARIANT_OLED)
SSD1306  display(0x3c, SDA, SCL);

#define LINES 6
#define LINE_LENGTH 21
char _lines[LINES][LINE_LENGTH + 1] = {"Weather Station", "", "", ""};

void fillLine(const char *message, int line_nr)
{
	snprintf(_lines[line_nr + 1], LINE_LENGTH, "%s", message);
}

void displayLines() {
	display.clear();
	for (int i = 0; i < LINES; i++)
  	display.drawString(0, 10 * i, _lines[i]);
  display.display();
}

void displayLine(const char * message, int line_nr) {
	fillLine(message, line_nr);
	displayLines();
}


#endif

void callback(char* topic, byte* payload, unsigned int length);

#define CONFIG_FILE "/config.json"
#define MY_NAME "serverName"
#define NETWORKS "networks"
bool loadConfig() {
#if defined(PUYA_ISSUE)
	return false;
#endif

	File configFile = SPIFFS.open(CONFIG_FILE, "r");
  if (!configFile) {
    return false;
  }

  size_t size = configFile.size();
	//displayLine(("open ok " + String(size)).c_str(), 4);
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
	//displayLine("json ok", 4);

	if (json.containsKey(MY_NAME))
  	myName = String((const char *)json[MY_NAME]);
	else
	{
		configFile.close();
		return false;
	}

	//displayLine(("myname: "+myName).c_str(), 4);

	if (json.containsKey(NETWORKS)){
		JsonObject& jo = json[NETWORKS];
		JsonObject::iterator I = jo.begin();
		while (I != jo.end())
		{
			OTA.addAP(I->key, I->value);
			//displayLine(I->key, 4);
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
  json[MY_NAME] = "myName";
	JsonObject& data = json.createNestedObject(NETWORKS);
	OTA.eachAP([](const String& ssid, const String& pw, void * ja){
		(*(JsonObject*)ja)[ssid] = pw;
	}, (void*)&data);

  json.printTo(configFile);

	//displayLine(("save ok " + String(configFile.size()) + " ").c_str(), -1);
	configFile.close();
  return true;
}

void setup() {
	dht11.begin();
#if defined(ESP_WEATHER_VARIANT_OLED)
	display.init();
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setColor(WHITE);
#else
	Wire.begin(SDA, SCL);
#endif
	bme.begin(0x76);

#if defined(ESP_WEATHER_VARIANT_OLED)
	displayLine("Initializing", 0);
#endif

	SPIFFS.begin();
	if (!loadConfig())
	{
		// Default config
		OTA.addAP(WIFI_SSID, WIFI_PASSWORD);
	}

	OTA.allowOpen(true);

	OTA.onConnect([](const String& ssid, EasyOTA::STATE state){
#if defined(ESP_WEATHER_VARIANT_OLED)
		displayLine("Connected", 0);
#endif
		if (myName == "")
		{
			myName = WiFi.macAddress();
			saveConfig();
		}
	});

	// Setup MqttClient
	client.setServer(MQTT_URL, MQTT_PORT);
  client.setCallback(callback);
}

void setMyName(const char * name)
{
	myName = String(name);
	saveConfig();
	#if defined(ESP_WEATHER_VARIANT_OLED)
			displayLine(myName.c_str(), 0);
	#endif
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

void deepsleep()
{
	if (client.connected())
		client.disconnect();
#if defined(ESP_WEATHER_VARIANT_OLED)
	displayLine("", 0);
#endif
	ESP.deepSleep(30L*60L*1000000L /*, optional RFMode mode*/);
}

void loop() {
	static long last_m = millis();
	static long last_m1 = last_m;
	static float _battery = 0, _temperature = 0, _humidity = 0, _pressure = 0;
	static bool _init = true;
	static bool _send = false;

	long now = millis();
	OTA.loop(now);
	client.loop();

	if (now - last_m1 > 1000)
	{
		float temp, humi, psi, voltage = 0;

		sensors_event_t event;
		dht11.humidity().getEvent(&event);

		humi = event.relative_humidity;

		for (int i = 0; i < 10; i++)
			voltage += analogRead(A0) / 1024.0;

		temp = bme.readTemperature();
		psi = bme.readPressure();

		_temperature += temp;
		_humidity += humi;
		_pressure += psi;
		_battery += voltage;

		if (!_init)
		{
			_temperature /= 2.0;
			_humidity /= 2.0;
			_pressure /= 2.0;
			_battery /= 2.0;
		} else
		{
			_init = false;
		}
		last_m1 = now;

		char str[21];

#if defined(ESP_WEATHER_VARIANT_OLED)
		sprintf(str, "T: %.1f", _temperature);
		fillLine(str, 1);
		sprintf(str, "P: %.1f", _pressure);
		fillLine(str, 2);
		sprintf(str, "H: %.1f", _humidity);
		fillLine(str, 3);
		displayLines();
#endif

		_send = true;
	}

  if (_send && client.connected())
	{
      // Once connected, publish an announcement...
			client.publish((myName + "/temperature").c_str(), String(_temperature).c_str(), true);
			client.publish((myName + "/battery").c_str(), String(_battery).c_str(), true);
			client.publish((myName + "/pressure").c_str(), String(_pressure).c_str(), true);
      client.publish((myName + "/humidity").c_str(), String(_humidity).c_str(), true);

			_send = false;
  } else if (_send)
	{
		// Attempt to connect
    if (client.connect("client", MQTT_ID, MQTT_PASSW)) {
			client.subscribe((myName + "/apadd").c_str());
			client.subscribe((myName + "/apremove").c_str());
			client.subscribe((myName + "/name").c_str());
			client.subscribe((myName).c_str());
			client.publish("announce", myName.c_str());
			#if defined(ESP_WEATHER_VARIANT_OLED)
					displayLine(myName.c_str(), 0);
			#endif
		} else {
			#if defined(ESP_WEATHER_VARIANT_OLED)
					displayLine("No MQTT", 0);
			#endif
			//if (OTA.state() == EasyOTA::EOS_STA)
				//last_m = now;
		}
	}

	if (_can_sleep && now - last_m > 30 * 1000)
	{
		// goto sleep
		deepsleep();
	}
}

void callback(char* topic, byte* payload, unsigned int length)
{
	char str[128] = "";

	memcpy(str, payload, min(sizeof(str), length));

	if (strcmp(topic, myName.c_str()) == 0){
		if (strcmp(str, "PING") == 0)
			client.publish("announce", myName.c_str());
		else if (strcmp(str, "NOSLEEP") == 0)
			_can_sleep = false;
		else if (strcmp(str, "SLEEP") == 0)
		 deepsleep();
	 else if (strcmp(str, "RESTART") == 0)
 		 ESP.restart();
	} else if (strcmp(topic, (myName + "/name").c_str()) == 0) {
		setMyName(str);
	} else if (strcmp(topic, (myName + "/apadd").c_str())) {
		char ssid[128] = "";
		char passw[128] = "";
		sscanf("%s %s", ssid, passw);
		addAP(ssid, passw);
	} else if (strcmp(topic, (myName + "/apremove").c_str())) {
		removeAP(str);
	}

	//displayLine(str, 4);
}
