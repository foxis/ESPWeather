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
#ifndef MY_SENSOR_NTP_H
#define MY_SENSOR_NTP_H

#include <SensorBase.h>
#include <NTPClient.h>

class SensorNTP : public SensorBase
{
  String serverIP;
  WiFiUDP ntpUDP;
  NTPClient timeClient;
  bool began;
public:
	SensorNTP() : timeClient(ntpUDP, "pool.ntp.org", 0, 1000) {
	}

	virtual void begin() {
    active = true;
    began = false;
    timeClient.setPoolServerName(ConfigurationBase::instance->ntp_url.c_str());
  }

  virtual void loop(unsigned long now, sensor_map_t & sensor_map) {
    if (!active || WiFi.status() != WL_CONNECTED)
      return;
    if (!began) {
      timeClient.begin();
      began = true;
    }

    if (!timeClient.forceUpdate()) {
      //active = false;
      return;
    }

    auto datetime = timeClient.getFormattedDate();
    SERIAL_V("Got NTP date&time: ");
    SERIAL_LN(datetime);

    if (sensor_map.find("rtc") == sensor_map.end())
      sensor_map["ntp"] = datetime;
    else {
      int splitT = datetime.indexOf("T");
      auto dayStamp = datetime.substring(0, splitT);
      auto timeStamp = datetime.substring(splitT + 1, datetime.length() - 1);
      ConfigurationBase::instance->telemetry.set_clock(dayStamp, timeStamp);
    }
    active = false;
  }
};

#endif
