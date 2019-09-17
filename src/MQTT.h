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
#ifndef MY_MQTT_H
#define MY_MQTT_H

#include "Config.h"
#include <PubSubClient.h>

class MQTT
{
	WiFiClient espClient;
	ConfigurationBase& config;
	PubSubClient client;
	bool announced;
	bool initialized;
	std::vector<String> listening;

public:
	static MQTT* instance;
	int readings;

	MQTT(ConfigurationBase& config) : config(config), client(espClient) {
		instance = this;
		announced = false;
		readings = 0;
		initialized = false;
	}

	void begin() {
		if (config.mqtt_url.length() == 0 || config.mqtt_user.length() == 0
				|| !config.wifi_enabled)
			return;

		// Setup MqttClient
		client.setServer(config.mqtt_url.c_str(), config.mqtt_port);
		initialized = true;
		client.setCallback([](char* topic, byte* payload, unsigned int length) {
			// This will receive MQTT configuration messages
			char str[128] = "";

			memcpy(str, payload, min(sizeof(str) - 1, length));

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
				char name[128] = "";
				char *src = str;
				char *dst = name;
				while (*src)
				{
					*dst = *(src++);
					if ((*dst == ',' || *dst == ' ' || *src == '\0') && dst > name) {
						if (*src == '\0') dst++;
						*dst = '\0';
						dst = name;
						MQTT::instance->listening.push_back(name);
					} else
						dst++;
				}

				MQTT::instance->config.setMyName(MQTT::instance->listening[0]);
				MQTT::instance->listening.erase(MQTT::instance->listening.begin());
#if defined(ESP_WEATHER_VARIANT_EPAPER)
				for (auto & name : MQTT::instance->listening) {
					MQTT::instance->client.subscribe((name + "/temperature").c_str());
					MQTT::instance->client.subscribe((name + "/pressure").c_str());
					MQTT::instance->client.subscribe((name + "/humidity").c_str());
					MQTT::instance->client.subscribe((name + "/battery").c_str());
				}
#endif
			} else if (strcmp(topic, (MQTT::instance->config.myName + "/apadd").c_str()) == 0) {
				char ssid[128] = "";
				char passw[128] = "";
				sscanf("%s %s", ssid, passw);
				MQTT::instance->config.addAP(ssid, passw);
			} else if (strcmp(topic, (MQTT::instance->config.myName + "/apremove").c_str()) == 0) {
				MQTT::instance->config.removeAP(str);
			} else
				MQTT::instance->handle_listen_stations(topic, str);
		});
		SERIAL_LN("MQTT ok");
	}

	void handle_listen_stations(const char * topic, const char * data) {
		std::vector<String>::iterator I = listening.begin();
		while (I != listening.end())
		{
			if (strcmp(topic, (*I + "/temperature").c_str()) == 0)
				config.display.publish_telemetry(*I, 0, atof(data), 0, 0, 0);
			else if (strcmp(topic, (*I + "/battery").c_str()) == 0)
				config.display.publish_telemetry(*I, atof(data), 0, 0, 0, 0);
			else if (strcmp(topic, (*I + "/pressure").c_str()) == 0)
				config.display.publish_telemetry(*I, 0, 0, 0, atof(data), 0);
			else if (strcmp(topic, (*I + "/humidity").c_str()) == 0)
				config.display.publish_telemetry(*I, 0, 0, atof(data), 0, 0);
			else if (strcmp(topic, (*I + "/light").c_str()) == 0)
				config.display.publish_telemetry(*I, 0, 0, 0, 0, atof(data));
			I++;
		}
	}

	void loop(unsigned long now) {
		if (!config.wifi_enabled) {
			if (config.telemetry._send){
				config.display.publish_status(config.woke_up ? "Wifi OFF" : "OTA / WebUI");
				config.display.publish_telemetry(config.myName, config.telemetry);
				config.telemetry._send = false;
				readings++;
			}
			return;
		}

		client.loop();

		if (config.OTA.state() == EasyOTA::EOS_STA)
		{
			if (!client.connected())
				reconnect();
			else {
				if (config.telemetry._send){
					if (!announced)
					{
						client.publish("announce", config.myName.c_str());
						config.display.publish_name(config.myName);
						announced = true;
					}
					publish_telemetry();
					config.display.publish_status(config.myName);
					config.display.publish_telemetry(config.myName, config.telemetry);
					config.telemetry._send = false;
				}
			}
		} else {
			if (config.telemetry._send) {
				config.display.publish_status(config.woke_up ? "No Wifi" : "OTA / WebUI");
				config.display.publish_telemetry(config.myName, config.telemetry);
				config.telemetry._send = false;
			}
		}
	}

	void reconnect() {
		if (config.mqtt_url.length() == 0 || config.mqtt_user.length() == 0
				|| !config.wifi_enabled) {
			config.display.publish_status("No MQTT");
			if (config.telemetry._send) {
				config.display.publish_telemetry(config.myName, config.telemetry);
				config.telemetry._send = false;
			}
			return;
		}

		String clientName = ESP_WEATHER_HOSTNAME + String("-") + config.myName;
		if (client.connect(clientName.c_str(), config.mqtt_user.c_str(), config.mqtt_password.c_str())) {
			// Once connected, subscribe to config topics
			client.subscribe((config.myName + "/apadd").c_str());
			client.subscribe((config.myName + "/apremove").c_str());
			client.subscribe((config.myName + "/name").c_str());
			client.subscribe((config.myName).c_str());
			SERIAL_LN("Conntected to MQTT");
		} else {
			config.display.publish_status("No MQTT");
			if (config.telemetry._send) {
				config.display.publish_telemetry(config.myName, config.telemetry);
				config.telemetry._send = false;
			}
		}
	}

	void disconnect(){
		if (client.connected())
			client.disconnect();
	}

	void publish_telemetry() {
		if (!initialized)
			return;

    // Publish telemetry data...
		for (auto & reading : config.telemetry.sensor_map)
			client.publish((config.myName + "/" + reading.first).c_str(), String(reading.second).c_str(), true);

		readings++;
	}
};

MQTT* MQTT::instance = NULL;

#endif
