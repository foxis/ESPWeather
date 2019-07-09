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
#ifndef MY_SENSOR_ADC_H
#define MY_SENSOR_ADC_H

#include <SensorBase.h>

#define AD7992_ADDR										(0x23)

#define AD7992_REG_CONVERSION_RESULT	(0x00)
#define AD7992_REG_CONFIGURATION			(0x02)
#define AD7992_REG_STATUS							(0x01)

class SensorADC : public SensorBase
{
public:
	SensorADC() {
	}

	virtual void begin() {
		active = test(&Wire, AD7992_ADDR);
		SERIAL_LN(active ? "AD7992 found!" : "No AD7992");
  }

  virtual void loop(unsigned long now, sensor_map_t & sensor_map) {
		float adc0, adc1;

		// channel 0
		write8(&Wire, AD7992_ADDR, AD7992_REG_CONFIGURATION | 0x10, 0b00011000);
		adc0 = read16(&Wire, AD7992_ADDR, AD7992_REG_CONVERSION_RESULT, MSBFirst) & 0x0FFF;

		// channel 1
		write8(&Wire, AD7992_ADDR, AD7992_REG_CONFIGURATION | 0x20, 0b00101000);
		adc1 = read16(&Wire, AD7992_ADDR, AD7992_REG_CONVERSION_RESULT, MSBFirst) & 0x0FFF;

		sensor_map["adc0"] = adc0 / 4095.0;
		sensor_map["adc1"] = adc1 / 4095.0;
  }
};

#endif
