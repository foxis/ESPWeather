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
#ifndef MY_SENSOR_BASE_H
#define MY_SENSOR_BASE_H

#include <ConfigBase.h>
#include <Arduino.h>
#include <Wire.h>
#include <map>

class SensorValue
{
  enum {
    SV_NONE,
    SV_FLOAT,
    SV_STRING,
  } _type;
  float f;
  String s;

public:
  SensorValue() {
    _type = SV_NONE;
  }
  SensorValue(float f) {
    _type = SV_FLOAT;
    this->f = f;
  }
  SensorValue(const String & str) {
    _type = SV_STRING;
    this->s = str;
  }
  SensorValue(const SensorValue& sv) {
    *this = sv;
  }

  void operator = (const SensorValue& sv) {
    _type = sv._type;
    f = sv.f;
    s = sv.s;
  }

  operator String () {
    switch (_type)
    {
      case SV_FLOAT:
        return String(f);
      case SV_STRING:
        return s;
      default:
        return "NA";
    }
  }

  float fvalue() {
    return f;
  }
};

typedef std::map<String, SensorValue> sensor_map_t;
typedef std::pair<String, SensorValue> sensor_val_t;

class SensorBase
{
public:
  enum ByteOrder {MSBFirst, LSBFirst};
  bool active;

	virtual void begin() = 0;
	virtual void loop(unsigned long now, sensor_map_t & sensor_map) = 0;

  static void discover(TwoWire * wire, bool * results, size_t num_results, uint8_t addr_start) {
		for (size_t i = addr_start; i < (addr_start + num_results); i++)
		{
			*(results++) = test(wire, i);
		}
	}

  static bool test(TwoWire * wire, uint8_t addr) {
    wire->beginTransmission(addr);
    return !wire->endTransmission();
  }

  virtual void read(TwoWire * wire, uint8_t addr, uint8_t reg, uint8_t * out, size_t max_len) {
    wire->beginTransmission(addr);
		if (reg != 0xFF) wire->write(reg);
		wire->endTransmission();
		wire->requestFrom(addr, max_len);
    while (max_len--)
      *(out++) = wire->read();
  }

  virtual void write(TwoWire * wire, uint8_t addr, uint8_t reg, const uint8_t * data, size_t len) {
    wire->beginTransmission(addr);
    wire->write(reg);
    if (data != NULL) wire->write(data, len);
    wire->endTransmission();
  }

  uint8_t read8(TwoWire * wire, uint8_t addr, uint8_t reg) {
    uint8_t data[1] = {reg};
    read(wire, addr, reg, data, sizeof(data));
    return data[0];
  }

	uint16_t read16(TwoWire * wire, uint8_t addr, uint8_t reg, ByteOrder order = LSBFirst) {
    uint8_t data[2] = {};
    read(wire, addr, reg, data, sizeof(data));
		if (order == LSBFirst)
    	return (uint16_t)(data[0]) | ((uint16_t)(data[1]) << 8);
		else
			return (uint16_t)(data[1]) | ((uint16_t)(data[0]) << 8);
  }

	uint32_t read24(TwoWire * wire, uint8_t addr, uint8_t reg, ByteOrder order = LSBFirst) {
    uint8_t data[3] = {};
    read(wire, addr, reg, data, sizeof(data));
		if (order == LSBFirst)
			return (uint32_t)(data[0]) | ((uint32_t)(data[1]) << 8) | ((uint32_t)(data[2]) << 16);
		else
			return (uint32_t)(data[2]) | ((uint32_t)(data[1]) << 8) | ((uint32_t)(data[0]) << 16);
  }

	uint32_t read32(TwoWire * wire, uint8_t addr, uint8_t reg, ByteOrder order = LSBFirst) {
    uint8_t data[4] = {};
    read(wire, addr, reg, data, sizeof(data));
		if (order == LSBFirst)
			return (uint32_t)(data[0]) | ((uint32_t)(data[1]) << 8) | ((uint32_t)(data[2]) << 16) | ((uint32_t)(data[3]) << 24);
		else
			return (uint32_t)(data[3]) | ((uint32_t)(data[2]) << 8) | ((uint32_t)(data[1]) << 16) | ((uint32_t)(data[0]) << 24);
  }

	void write8(TwoWire * wire, uint8_t addr, uint8_t reg, uint8_t d) {
		uint8_t data[] = {d};
		write(wire, addr, reg, data, sizeof(data));
	}

	void write8(TwoWire * wire, uint8_t addr, uint8_t d) {
		write(wire, addr, d, NULL, 0);
	}

	void write16(TwoWire * wire, uint8_t addr, uint8_t reg, uint16_t d, ByteOrder order = LSBFirst) {
		if (order == LSBFirst) {
			uint8_t data[] = {(uint8_t)d, (uint8_t)(d >> 8)};
			write(wire, addr, reg, data, sizeof(data));
		}
		else {
			uint8_t data[] = {(uint8_t)(d >> 8), (uint8_t)d};
			write(wire, addr, reg, data, sizeof(data));
		}
	}

	void write24(TwoWire * wire, uint8_t addr, uint8_t reg, uint32_t d, ByteOrder order = LSBFirst) {
		if (order == LSBFirst) {
			uint8_t data[] = {(uint8_t)d, (uint8_t)(d >> 8), (uint8_t)(d >> 16)};
			write(wire, addr, reg, data, sizeof(data));
		}
		else {
			uint8_t data[] = {(uint8_t)(d >> 16), (uint8_t)(d >> 8), (uint8_t)d};
			write(wire, addr, reg, data, sizeof(data));
		}
	}

	void write32(TwoWire * wire, uint8_t addr, uint8_t reg, uint32_t d, ByteOrder order = LSBFirst) {
		if (order == LSBFirst) {
			uint8_t data[] = {(uint8_t)d, (uint8_t)(d >> 8), (uint8_t)(d >> 16), (uint8_t)(d >> 24)};
			write(wire, addr, reg, data, sizeof(data));
		}
		else {
			uint8_t data[] = {(uint8_t)(d >> 24), (uint8_t)(d >> 16), (uint8_t)(d >> 8), (uint8_t)d};
			write(wire, addr, reg, data, sizeof(data));
		}
	}

  bool wait_while(TwoWire * wire, uint8_t addr, uint8_t reg, uint8_t mask, uint8_t result, unsigned long timeout_us) {
		unsigned long now = micros();
	  while ((read8(wire, addr, reg) & mask) == result)
	  {
	    if (timeout_us > 0 && (micros() - now > timeout_us))
	      return false;
	  }
		return true;
	}

	bool wait_until(TwoWire * wire, uint8_t addr, uint8_t reg, uint8_t mask, uint8_t result, unsigned long timeout_us) {
		unsigned long now = micros();
	  while ((read8(wire, addr, reg) & mask) != result)
	  {
			if (timeout_us > 0 && (micros() - now > timeout_us))
	      return false;
	  }
		return true;
	}

};

#endif
