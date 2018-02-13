#include "wificonfig.h"
#include <EasyOTA.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>

#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_BMP280.h>

#include <SSD1306.h>
#include <PubSubClient.h>


EasyOTA OTA(ARDUINO_HOSTNAME);
DHT_Unified dht11(02, DHT11);
Adafruit_BMP280 bme;
WiFiClient espClient;
PubSubClient client(espClient);

#define SDA  1
#define SCL 3

#if defined(ESP_WEATHER_VARIANT_OLED)
SSD1306  display(0x3c, SDA, SCL);

void displayLine(char *message, int line_nr) {
	#define LINES 5
	#define LINE_LENGTH 21

  static char lines[LINES][LINE_LENGTH + 1] = {"Weather Station", "", "", ""};

  snprintf(lines[line_nr + 1], LINE_LENGTH, "%s", message);

	display.clear();
	for (int i = 0; i < LINES; i++)
  	display.drawString(0, 10 * i, lines[i]);
  display.display();

	#undef LINES
	#undef LINE_LENGTH
}
#endif

void callback(char* topic, byte* payload, unsigned int length);
void send_data(float volts, float temp, float humi, float psi);

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

	OTA.addAP(WIFI_SSID, WIFI_PASSWORD);
	OTA.allowOpen(true);

	OTA.onConnect([](const String& ssid, EasyOTA::STATE state){
#if defined(ESP_WEATHER_VARIANT_OLED)
		displayLine("Connected", 0);
#endif
		// TODO:
		// setup mqtt client
		// send data
		//Serial.println("Connection changed: " + ssid + ";" + String(state));

	});

	// Setup MqttClient
	client.setServer(MQTT_URL, MQTT_PORT);
  client.setCallback(callback);
}

void loop() {
	static long last_m = millis();
	static long last_m1 = last_m;
	static float _volts = 0, _temp = 0, _humi = 0, _psi = 0;
	static bool _init = true;
	static bool _send = false;

	long now = millis();
	OTA.loop(now);
	client.loop();

	if (now - last_m1 > 1000)
	{
		float temp, humi, psi;

		sensors_event_t event;
		dht11.temperature().getEvent(&event);

		temp = event.temperature;

		dht11.humidity().getEvent(&event);

		humi = event.relative_humidity;

		float volts = 0;
		for (int i = 0; i < 10; i++)
			volts += analogRead(A0)/102.4;
		volts /= 10.0;
		temp = (temp + bme.readTemperature()) / 2.0;
		psi = bme.readPressure();

		_temp += temp;
		_humi += humi;
		_psi += psi;
		_volts += volts;

		if (!_init)
		{
			_temp /= 2.0;
			_humi /= 2.0;
			_psi /= 2.0;
			_volts /= 2.0;
		} else
		{
			_init = false;
		}
		last_m1 = now;

		char str[21];

#if defined(ESP_WEATHER_VARIANT_OLED)
		sprintf(str, "T: %.1f", _temp);
		displayLine(str, 1);
		sprintf(str, "P: %.1f", _psi);
		displayLine(str, 2);
		sprintf(str, "H: %.1f", _humi);
		displayLine(str, 3);
#endif

		_send = true;
	}

  if (_send && client.connected())
	{
      // Once connected, publish an announcement...
			String mac = WiFi.macAddress();
			client.publish((mac+"/temp").c_str(), String(_temp).c_str());
			client.publish((mac+"/volt").c_str(), String(_volts).c_str());
			client.publish((mac+"/pressure").c_str(), String(_psi).c_str());
      client.publish((mac+"/hum").c_str(), String(_humi).c_str());

			_send = false;
      // ... and resubscribe
      //client.subscribe("inTopic");
  } else if (_send)
	{
		// Attempt to connect
    if (client.connect("client", MQTT_ID, MQTT_PASSW)) {
		} else {
		}
	}

	if (now - last_m > 30 * 1000)
	{
		// goto sleep
		if (client.connected())
			client.disconnect();
		ESP.deepSleep(30*60*10000000/*, optional RFMode mode*/);
	}
}

void callback(char* topic, byte* payload, unsigned int length)
{

}
