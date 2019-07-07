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
#ifndef MY_SENSOR_BME_H
#define MY_SENSOR_BME_H

#include <SensorBase.h>
#include <BME280I2C.h>

class SensorBME : public SensorBase
{
  BME280I2C bme;
public:
	SensorBME() {
	}

	virtual void begin() {
		active = bme.begin();
    //Serial.println(active ? "BME280 found!" : "No BME280");
  }

  virtual void loop(unsigned long now, sensor_map_t & sensor_map) {
    float temp, humi, psi;

    BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
    BME280::PresUnit presUnit(BME280::PresUnit_Pa);

    bme.read(psi, temp, humi, tempUnit, presUnit);

    sensor_map["temperature"] = temp;
    sensor_map["humidity"] = humi;
    sensor_map["pressure"] = psi;
  }
};

#endif
