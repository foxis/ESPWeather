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

#include <SensorBase.h>

class Mutex {
	volatile bool _lock;

public:
	Mutex() {
		_lock = false;
	}

	void lock() {
		while (_lock) {}
		_lock = true;
	}

	void unlock() {
		_lock = false;
	}
};

class TelemetryBase : public Mutex
{
public:
	TelemetryBase() {
		_send = false;
		build_json = false;
	}

	virtual void begin() = 0;
	virtual void loop(unsigned long now) = 0;
	virtual void set_clock(const String & date, const String & time) = 0;

	bool _send;
	sensor_map_t sensor_map;
	String json;
	bool build_json;
};

#endif
