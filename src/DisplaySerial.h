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
#ifndef MY_DISPLAY_SERIAL_H
#define MY_DISPLAY_SERIAL_H

#define Display DisplayOLED
#include "DisplayOLED.h"
#undef Display
#include <Arduino.h>
#include <SoftwareSerial.h>

class Display : public DisplayOLED
{
	SoftwareSerial logger;
	bool available;
public:
	Display(ConfigurationBase& config) :
		DisplayOLED(config, SDA0, SCL0),
		logger(SCL1, SDA1, false, 128, 10) {
		available = false;
	}

	virtual void begin() {
		DisplayOLED::begin();
		logger.begin(9600);
	}
	virtual void end() {
		DisplayOLED::end();
	};

	virtual void publish_telemetry(const String& name, TelemetryBase& t) {
		DisplayBase::publish_telemetry(name, t);
		for (auto & reading : t.sensor_map) {
			logger.print(reading.first);
			logger.print("=");
			logger.print(reading.second);
			logger.print(",");

			Serial.print(reading.first);
			Serial.print("=");
			Serial.print(reading.second);
			Serial.print(",");
		}
		logger.println();
		Serial.println();
	}
};

#endif
