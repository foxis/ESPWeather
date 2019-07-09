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
#ifndef MY_DISPLAY_BASE_H
#define MY_DISPLAY_BASE_H

#include <Arduino.h>
#include <TelemetryBase.h>

class DisplayBase
{
public:
	virtual void begin() = 0;
	virtual void end() = 0;
	virtual void loop(unsigned long now) = 0;

	virtual void publish_telemetry(const String& name, TelemetryBase& t) {
		float battery, temp, humi, pressure, light;
		battery = t.sensor_map["battery"].fvalue();
		temp = t.sensor_map["temperature"].fvalue();
		humi = t.sensor_map["humidity"].fvalue();
		pressure = t.sensor_map["pressure"].fvalue();
		light = t.sensor_map["light"].fvalue();
		publish_telemetry(name, battery, temp, humi, pressure, light);
	}

	virtual void publish_telemetry(const String& name, float battery, float temp, float humidity, float pressure, float light) = 0;
	virtual void publish_name(const String& name) = 0;
	virtual void publish_status(const String& str) = 0;
	virtual void log(const String& str) = 0;
};

#endif
