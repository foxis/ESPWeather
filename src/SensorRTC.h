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
  };

  template <typename T>
  class RTCWrapper : public RTCWrapperBase
  {
    T rtc;
    String name;
  public:
    RTCWrapper(const String & name) {
      this->name = name;
    }

    virtual String found_name() const {
      return name + " Found!";
    }
    virtual String now() {
      return rtc.now().timestamp();
    }

    virtual bool detect();
  };

  std::vector<RTCWrapperBase *> clocks;
  RTCWrapperBase * rtc;
public:
	SensorRTC() {
    clocks.push_back(new RTCWrapper<RTC_DS1307>("DS1307"));
    clocks.push_back(new RTCWrapper<RTC_DS3231>("DS3231"));
    clocks.push_back(new RTCWrapper<RTC_PCF8523>("PCF8523"));
    rtc = NULL;
	}

	virtual void begin() {
    for (auto & clock : clocks)
      if (clock->detect()) {
        Serial.println(clock->found_name());
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
};

template<>
bool SensorRTC::RTCWrapper<RTC_DS1307>::detect() {
  return rtc.begin() && rtc.isrunning();
}
template<>
bool SensorRTC::RTCWrapper<RTC_DS3231>::detect() {
  return rtc.begin();
}
template<>
bool SensorRTC::RTCWrapper<RTC_PCF8523>::detect() {
  return rtc.begin() && rtc.initialized();
}

#endif
