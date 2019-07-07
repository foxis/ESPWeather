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
#ifndef MY_SENSOR_UV_H
#define MY_SENSOR_UV_H

#include <SensorBase.h>
#include <Adafruit_SI1145.h>

class SensorUV : public SensorBase
{
  Adafruit_SI1145 uv;
public:
	SensorUV() {
	}

	virtual void begin() {
    active = uv.begin();
    //Serial.println(active ? "SI1145 found!" : "No SI1145");
  }

  virtual void loop(unsigned long now, sensor_map_t & sensor_map) {
    if (!active)
      return;

    sensor_map["light"] = uv.readVisible();
    sensor_map["uv-index"] = uv.readUV() / 100.0;
  }
};

#endif
