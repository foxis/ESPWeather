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
