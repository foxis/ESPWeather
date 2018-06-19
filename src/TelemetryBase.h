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
#ifndef MY_TELEMETRY_BASE_H
#define MY_TELEMETRY_BASE_H

class TelemetryBase
{
public:
	TelemetryBase() {
		_send = false;
		_battery = _temperature = _humidity = _pressure = _light = 0;
	}

	virtual void begin() = 0;
	virtual void loop(unsigned long now) = 0;

	float _battery, _temperature, _humidity, _pressure, _light;
	bool _send;
};

#endif
