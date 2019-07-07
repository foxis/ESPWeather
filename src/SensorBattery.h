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
#ifndef MY_SENSOR_BATTERY_H
#define MY_SENSOR_BATTERY_H

#include <SensorBase.h>

#define _R6 249.0
#define _R5 51.0
#define BATTERY_DIVIDER_COEFFICIENT (1.0 / (_R5 / (_R5 + _R6)))

class SensorBattery : public SensorBase
{
public:
	SensorBattery() {
	}

	virtual void begin() {
    #if !defined(ESP_WEATHER_VARIANT_PRO)
    pinMode(SENSOR_SWITCH, OUTPUT);
    #endif
    pinMode(A0, INPUT);
  }

  virtual void loop(unsigned long now, sensor_map_t & sensor_map) {
    float battery = readAnalog(BATTERY_DIVIDER_COEFFICIENT, 20, false);

    sensor_map["battery"] = battery;
  }

  float readAnalog(float c, int N, bool sw) {
		float analog = 0;
    #if !defined(ESP_WEATHER_VARIANT_PRO)
		digitalWrite(SENSOR_SWITCH, sw);
    #endif
		for (int i = 0; i < N; i++)
			analog += c * analogRead(A0) / 1024.0;
		return analog / (float)N;
	}
};

#endif
