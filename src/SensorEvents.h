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
#ifndef MY_SENSOR_EVENTS_H
#define MY_SENSOR_EVENTS_H

#include <SensorBase.h>
#include <Wire.h>

#define DS1682_ADDR 0x6B
#define EVENT_COUNTER 0x09

class SensorEvents : public SensorBase
{
public:
	SensorEvents() {
	}

	virtual void begin() {
    active = test(&Wire, DS1682_ADDR);
    SERIAL_LN(active ? "DS1682 found!" : "No DS1682");
  }

  virtual void loop(unsigned long now, sensor_map_t & sensor_map) {
    sensor_map["events"] = read16(&Wire, DS1682_ADDR, EVENT_COUNTER);
  }
};

#endif
