/**
    Default configuration of the Weather Station
		Copy this file to wificonfig.h and edit appropriately.

		NOTE: To configure station name, add networks one must be
		      connected to proper mqtt broker.
					Such configuration can be done if ESP_WEATHER_VARIANT_UI defined though.
 */


// Select one of the variants
#define ESP_WEATHER_VARIANT_OLED
//#define ESP_WEATHER_VARIANT_EPAPER
#define ESP_WEATHER_VARIANT_HEADLESS
// This will enable Web UI
//#define ESP_WEATHER_VARIANT_UI

// Setup Wifi networks
#define WIFI_SSID        "ssid"
#define WIFI_PASSWORD    "passwd"

#define MQTT_ID					"mqtt-user"
#define MQTT_PASSW			"mqtt-passw"
#define MQTT_PORT				16769
#define MQTT_URL				"m23.cloudmqtt.com"

#if defined(ESP_WEATHER_VARIANT_HEADLESS)
#define ARDUINO_HOSTNAME "ESPWeather"
#elif defined(ESP_WEATHER_VARIANT_EPAPER)
#elif defined(ESP_WEATHER_VARIANT_OLED)
#define ARDUINO_HOSTNAME "ESPWeather-Display"
#else
#error Weather station variant not supported
#endif

#if defined(ESP_WEATHER_VARIANT_UI)
#undef ARDUINO_HOSTNAME
#define ARDUINO_HOSTNAME "ESPWeather-UI"
#endif
