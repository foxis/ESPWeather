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
#ifndef MY_SENSOR_RTC_H
#define MY_SENSOR_RTC_H

#include <SensorBase.h>
#include "RTClib.h"
#include <vector>

class SensorRTC : public SensorBase
{
  class RTCWrapperBase
  {
  public:
    virtual String found_name() const = 0;
    virtual String now() = 0;
    virtual bool detect() = 0;
    virtual void set(const String & date, const String & time) = 0;
  };

  template <typename T>
  class RTCWrapper : public RTCWrapperBase
  {
    uint8_t addr;
    T rtc;
    String name;
  public:
    RTCWrapper(const String & name, uint8_t addr) {
      this->name = name;
      this->addr = addr;
    }

    virtual String found_name() const {
      return name + " Found!";
    }
    virtual String now() {
      return rtc.now().timestamp();
    }

    virtual bool detect() {
      return SensorBase::test(&Wire, addr) && rtc.begin() && init();
    }

    virtual void set(const String & date, const String & time) {
      rtc.adjust(DateTime(date.c_str(), time.c_str()));
    }
    bool init();
  };

  std::vector<RTCWrapperBase *> clocks;
  RTCWrapperBase * rtc;
public:
	SensorRTC() {
    clocks.push_back(new RTCWrapper<RTC_DS1307>("DS1307", DS1307_ADDRESS));
    clocks.push_back(new RTCWrapper<RTC_DS3231>("DS3231", DS3231_ADDRESS));
    clocks.push_back(new RTCWrapper<RTC_PCF8523>("PCF8523", PCF8523_ADDRESS));
    rtc = NULL;
	}

	virtual void begin() {
    for (auto & clock : clocks)
      if (clock->detect()) {
        SERIAL_LN(clock->found_name());
        rtc = clock;
        break;
      }
    active = rtc != NULL;
  }

  virtual void loop(unsigned long now, sensor_map_t & sensor_map) {
    if (rtc == NULL)
      return;

    sensor_map["rtc"] = rtc->now();
  }

  virtual void set_time(const String & date, const String & time) {
    if (rtc != NULL)
      rtc->set(date, time);
  }

};

template<>
bool SensorRTC::RTCWrapper<RTC_DS1307>::init() {
  if (!rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  return true;
}

template<>
bool SensorRTC::RTCWrapper<RTC_DS3231>::init() {
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  return true;
}

template<>
bool SensorRTC::RTCWrapper<RTC_PCF8523>::init() {
  if (!rtc.initialized()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  return true;
}

#endif
