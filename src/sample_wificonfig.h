/**

MIT License

Copyright (c) 2018 foxis (Andrius Mikonis <andrius.mikonis@gmail.com>)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

 **/

/**
    Default configuration of the Weather Station
		Copy this file to wificonfig.h and edit appropriately.

		NOTE: To configure station name, add networks one must be
		      connected to proper mqtt broker.
					Such configuration can be done if ESP_WEATHER_VARIANT_UI defined though.
 */
 #ifndef MY_WIFICONFIG_H
 #define MY_WIFICONFIG_H

// Select one of the variants or none for headless variant
//#define ESP_WEATHER_VARIANT_OLED
//#define ESP_WEATHER_VARIANT_EPAPER
// This will enable Web UI
//#define ESP_WEATHER_VARIANT_UI

// Setup Wifi networks
#define WIFI_SSID        "ssid"
#define WIFI_PASSWORD    "passwd"

// Define these for secondary WiFi network
//#define WIFI_SSID1        "ssid"
//#define WIFI_PASSWORD1    "passwd"

#define MQTT_ID					"mqtt-user"
#define MQTT_PASSW			"mqtt-passw"
#define MQTT_PORT				16769
#define MQTT_URL				"m23.cloudmqtt.com"

#if defined(ESP_WEATHER_VARIANT_EPAPER)
#define ARDUINO_HOSTNAME "ESPWeather-epaper"
#elif defined(ESP_WEATHER_VARIANT_OLED)
#define ARDUINO_HOSTNAME "ESPWeather-oled"
#else
// ESP_WEATHER_VARIANT_HEADLESS
#define ARDUINO_HOSTNAME "ESPWeather"
#endif

#if defined(ESP_WEATHER_VARIANT_UI)
#undef ARDUINO_HOSTNAME
#define ARDUINO_HOSTNAME "ESPWeather-UI"
#endif

#endif
