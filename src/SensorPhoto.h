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
#ifndef MY_SENSOR_PHOTO_H
#define MY_SENSOR_PHOTO_H

#include <SensorBattery.h>

#define _R7 100.0
#define _R8 4.99
#define LIGHT_THRESHOLD .5
#define LIGHT_1_DIVIDER_COEFFICIENT (1.0 / (1.0))
#define LIGHT_2_DIVIDER_COEFFICIENT (1.0 / (1.0))
#define LIGHT_BIAS (3.3 * _R8 / (_R8 + (3.3 - LIGHT_THRESHOLD) * _R7 / LIGHT_THRESHOLD))

class SensorPhoto : public SensorBattery
{
public:
	SensorPhoto() {
	}

	virtual void begin() {
    SensorBattery::begin();
  }

  virtual void loop(unsigned long now, sensor_map_t & sensor_map) {
		// set analog switch for light sensor
		// 100k pulldown
		pinMode(DIVIDER_SELECTOR, INPUT);
		digitalWrite(DIVIDER_SELECTOR, LOW); // no pullup
		float light = readAnalog(LIGHT_1_DIVIDER_COEFFICIENT, 20, true);

		if (light >= LIGHT_THRESHOLD) {
			pinMode(DIVIDER_SELECTOR, OUTPUT);
			digitalWrite(DIVIDER_SELECTOR, LOW);
			light = LIGHT_THRESHOLD + readAnalog(LIGHT_2_DIVIDER_COEFFICIENT, 20, true) - LIGHT_BIAS;
			pinMode(DIVIDER_SELECTOR, INPUT);
			digitalWrite(DIVIDER_SELECTOR, LOW); // no pullup
		}

		sensor_map["light"] = light;
  }

};

#endif
