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
#endif


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
		#endif
	}

	virtual void begin() {
		_init = true;

		//discover();

		for (auto & sensor : sensors)
			sensor->begin();

		last_m1 = millis();
	}

	void discover() {
		bool results[255] = {false,};
		Serial.println();
		Serial.println("Searching I2C bus 0...");
		SensorBase::discover(&Wire, results, 255, 0);
		for (int i = 0; i < 255; i++)
			if (results[i]) {
				Serial.print("Device found at: 0x");
				Serial.println(i, HEX);
			}
		Serial.println("Searching I2C bus 1...");
		memset(results, 0, sizeof(results));
		SensorBase::discover(&Wire1, results, 255, 0);
		for (int i = 0; i < 255; i++)
			if (results[i]) {
				Serial.print("Device found at: 0x");
				Serial.println(i, HEX);
			}
	}

	virtual void loop(unsigned long now) {
		// perform measurements every second
		if (now - last_m1 > 1000) {
			for (auto & sensor : sensors)
				sensor->loop(now, sensor_map);
			_send = true;
			last_m1 = now;
		}
	}
};

#endif
