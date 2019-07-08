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
#ifndef MY_SENSOR_DALLAS_H
#define MY_SENSOR_DALLAS_H

#include <SensorBase.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <vector>

class SensorDallas : public SensorBase
{
  class AddressWrapper
  {
  public:
    DeviceAddress addr;
    AddressWrapper() {}

    uint8_t operator[](size_t i) const {
      return addr[i];
    }
  };

  OneWire ds;
  DallasTemperature sensors;
  std::vector<AddressWrapper> thermometers;
public:
	SensorDallas() : ds(ONE_WIRE_BUS), sensors(&ds) {
	}

	virtual void begin() {
    int devices = sensors.getDeviceCount();
    active = devices > 0;
    for (int i = 0; i < devices; i ++) {
          AddressWrapper tmp;
          if (!sensors.getAddress(tmp.addr, i))
            thermometers.push_back(tmp);
    }
    //Serial.println(active ? "SI1145 found!" : "No SI1145");
  }

  virtual void loop(unsigned long now, sensor_map_t & sensor_map) {
    char addr[17] = "";
    for (auto & sensor : thermometers) {
      float T = sensors.getTempC(sensor.addr);
      for (int i = 0; i < 8; i++) {
        uint8_t tmp = sensor[i];
        addr[i * 2] = '0' + ((tmp & 0xF0) >> 4);
        addr[i * 2 + 1] = '0' + (tmp & 0x0F);
      }
      sensor_map[String("T") + addr] = T;
    }
  }
};

#endif
