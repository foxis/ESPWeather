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
#include <BME280I2C.h>

#define SENSOR_SWITCH 2
#define DIVIDER_SELECTOR 5
#define _R9 100.0
#define _R8 5.0
#define _R5 240.0
#define _R4 59.0
#define LIGHT_THRESHOLD .5
#define BATTERY_DIVIDER_COEFFICIENT (1.0 / (_R4 / (_R5 + _R4)))
#define LIGHT_1_DIVIDER_COEFFICIENT (1.0 / (1.0))
#define LIGHT_2_DIVIDER_COEFFICIENT (1.0 / (1.0))
#define LIGHT_BIAS (3.3 * _R8 / (_R8 + (3.3 - LIGHT_THRESHOLD) * _R9 / LIGHT_THRESHOLD))
// These will come from EasyOTA
//#define GETTER(T, name) T name() { return _##name; }
//#define SETTER(T, name) T name(T name) { T pa##name = _##name; _##name = name; return pa##name; }

class Telemetry : public TelemetryBase
{
	BME280I2C bme;
	ConfigurationBase& config;
	unsigned long last_m1;
	bool _init;
	int _skip_readings;

public:
	Telemetry(ConfigurationBase& config) :
		TelemetryBase(),
		config(config)
	{

	}

	virtual void begin() {
		Wire.begin(SDA, SCL);
		bme.begin();

		_init = true;
		last_m1 = millis();
		pinMode(SENSOR_SWITCH, OUTPUT);
	}

	float readAnalog(float c, int N, bool sw) {
		float analog = 0;
		digitalWrite(SENSOR_SWITCH, sw);
		for (int i = 0; i < N; i++)
			analog += c * analogRead(A0) / 1024.0;
		return analog / (float)N;
	}

	virtual void loop(unsigned long now) {
		// perform measurements every second
		if (now - last_m1 > 1000) {
			float temp, humi, psi, light;

			// set analog switch for battery
			_battery += readAnalog(BATTERY_DIVIDER_COEFFICIENT, 20, false);

			// set analog switch for light sensor
			// 100k pulldown
			pinMode(DIVIDER_SELECTOR, INPUT);
			digitalWrite(DIVIDER_SELECTOR, LOW); // no pullup
			light = readAnalog(LIGHT_1_DIVIDER_COEFFICIENT, 20, true);

			if (light >= LIGHT_THRESHOLD) {
				pinMode(DIVIDER_SELECTOR, OUTPUT);
				digitalWrite(DIVIDER_SELECTOR, LOW);
				light = LIGHT_THRESHOLD + readAnalog(LIGHT_2_DIVIDER_COEFFICIENT, 20, true) - LIGHT_BIAS;
				pinMode(DIVIDER_SELECTOR, INPUT);
				digitalWrite(DIVIDER_SELECTOR, LOW); // no pullup
			}

			_light += light;

			// Read BME280 measurements
			BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
	    BME280::PresUnit presUnit(BME280::PresUnit_Pa);

	    bme.read(psi, temp, humi, tempUnit, presUnit);

			// NOTE: Will perform humidity and battery voltage measurements up til now
			if (_temperature == 0) {
				_pressure = psi * 2;
				_temperature = temp * 2;
				_humidity = humi * 2;
				_send = true;
			} else {
				_pressure += psi;
				_temperature += temp;
				_humidity += humi;
				_send = true;
			}

			// first time measurements
			if (!_init)  {
				_pressure /= 2.0;
				_temperature /= 2.0;
				_humidity /= 2.0;
				_battery /= 2.0;
				_light /= 2.0;
			} else {
				_init = false;
			}

			last_m1 = now;
		}
	}
};

#endif
