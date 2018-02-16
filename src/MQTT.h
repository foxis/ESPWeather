#ifndef MY_MQTT_H
#define MY_MQTT_H

#include "Config.h"
#include <PubSubClient.h>

class MQTT
{
public:
	static MQTT* instance;

	WiFiClient espClient;
	PubSubClient client;
	ConfigurationBase& config;

	MQTT(ConfigurationBase& config) : config(config), client(espClient) {
		instance = this;
	}

	void begin() {
		// Setup MqttClient
		client.setServer(MQTT_URL, MQTT_PORT);
		client.setCallback([](char* topic, byte* payload, unsigned int length) {
			// This will receive MQTT configuration messages
			char str[128] = "";

			memcpy(str, payload, min(sizeof(str), length));

			if (strcmp(topic, MQTT::instance->config.myName.c_str()) == 0){
				if (strcmp(str, "PING") == 0)
					MQTT::instance->client.publish("announce", MQTT::instance->config.myName.c_str());
				else if (strcmp(str, "NOSLEEP") == 0)
					MQTT::instance->config.can_sleep = false;
				else if (strcmp(str, "SLEEP") == 0)
				 MQTT::instance->config.deepsleep();
			 else if (strcmp(str, "RESTART") == 0)
		 		 MQTT::instance->config.restart();
			} else if (strcmp(topic, (MQTT::instance->config.myName + "/name").c_str()) == 0) {
				MQTT::instance->config.setMyName(str);
			} else if (strcmp(topic, (MQTT::instance->config.myName + "/apadd").c_str())) {
				char ssid[128] = "";
				char passw[128] = "";
				sscanf("%s %s", ssid, passw);
				MQTT::instance->config.addAP(ssid, passw);
			} else if (strcmp(topic, (MQTT::instance->config.myName + "/apremove").c_str())) {
				MQTT::instance->config.removeAP(str);
			}
		});
	}

	void loop(long now) {
		client.loop();

		if (config.OTA.state() == EasyOTA::EOS_STA)
		{
			if (!client.connected())
				reconnect();
			else {
				if (config.telemetry._send){
					publish_telemetry();
					config.display.publish_telemetry(config.myName);
					config.telemetry._send = false;
					config.display.publish_status(config.myName);
				}
			}
		}
	}

	void reconnect() {
		if (client.connect(ARDUINO_HOSTNAME, MQTT_ID, MQTT_PASSW)) {
			// Once connected, subscribe to config topics
			client.subscribe((config.myName + "/apadd").c_str());
			client.subscribe((config.myName + "/apremove").c_str());
			client.subscribe((config.myName + "/name").c_str());
			client.subscribe((config.myName).c_str());
			// Announce self name
			client.publish("announce", config.myName.c_str());

			config.display.publish_name(config.myName);
		} else {
			//config.display.publish_status("No MQTT");
		}
	}

	void disconnect(){
		if (client.connected())
			client.disconnect();
	}

	void publish_telemetry() {
    // Publish telemetry data...
		client.publish((config.myName + "/temperature").c_str(), String(config.telemetry._temperature).c_str(), true);
		client.publish((config.myName + "/battery").c_str(), String(config.telemetry._battery).c_str(), true);
		client.publish((config.myName + "/pressure").c_str(), String(config.telemetry._pressure).c_str(), true);
    client.publish((config.myName + "/humidity").c_str(), String(config.telemetry._humidity).c_str(), true);
	}
};

MQTT* MQTT::instance = NULL;

#endif
