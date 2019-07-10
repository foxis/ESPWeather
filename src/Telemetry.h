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
#ifndef MY_TELEMETRY_H
#define MY_TELEMETRY_H

#include "Config.h"
#include "TelemetryBase.h"
#include <vector>

#include <SensorBattery.h>
#include <SensorPhoto.h>
#include <SensorBME.h>
#if defined(ESP_WEATHER_VARIANT_PRO)
#include <SensorEvents.h>
#include <SensorADC.h>
#include <SensorUV.h>
#include <SensorDallas.h>
#include <SensorRTC.h>
#endif
#include <SensorNTP.h>


class Telemetry : public TelemetryBase
{
	ConfigurationBase& config;
	unsigned long last_m1;
	bool _init;
	int _skip_readings;
	std::vector<SensorBase*> sensors;

public:
	Telemetry(ConfigurationBase& config) :
		TelemetryBase(),
		config(config)
	{
		sensors.push_back(new SensorBattery());
		sensors.push_back(new SensorBME());
		#if !defined(ESP_WEATHER_VARIANT_PRO)
		sensors.push_back(new SensorPhoto());
		#else
		sensors.push_back(new SensorEvents());
		sensors.push_back(new SensorADC());
		sensors.push_back(new SensorUV());
		sensors.push_back(new SensorDallas());
		sensors.push_back(new SensorRTC());
		#endif
		sensors.push_back(new SensorNTP());
	}

	virtual void begin() {
		_init = true;

		#if defined(ESP_WEATHER_VARIANT_PRO)
		discover();
		#endif

		for (auto & sensor : sensors)
			sensor->begin();

		last_m1 = millis();
	}

	void discover() {
		bool results[255] = {false,};
		SERIAL_LN("Searching I2C bus 0...");
		SensorBase::discover(&Wire, results, 255, 0);
		for (int i = 0; i < 255; i++)
			if (results[i]) {
				SERIAL_V("Device found at: 0x");
				SERIAL_LN1(i, HEX);
			}
		//Serial.println("Searching I2C bus 1...");
		//memset(results, 0, sizeof(results));
		//SensorBase::discover(&Wire1, results, 255, 0);
		//for (int i = 0; i < 255; i++)
		//	if (results[i]) {
		//		Serial.print("Device found at: 0x");
		//		Serial.println(i, HEX);
		//	}
	}

	virtual void loop(unsigned long now) {
		// perform measurements every second
		if (now - last_m1 > 1000) {
			for (auto & sensor : sensors)
				sensor->loop(now, sensor_map);
			_send = true;
			last_m1 = now;

			if (build_json) {
				String tmp;
				StaticJsonBuffer<1024> buf;
				JsonObject &obj = buf.createObject();
				for (auto & reading : sensor_map) {
					obj[reading.first] = (String)reading.second;
				}
				obj.printTo(tmp);

				lock();
				this->json = tmp;
				unlock();
			}
		}
	}

	virtual void set_clock(const String & date, const String & time) {
		for (auto & sensor : sensors)
			sensor->set_time(date, time);
	}

};

#endif
