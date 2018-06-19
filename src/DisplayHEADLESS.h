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
#ifndef MY_DISPLAY_HEADLESS_H
#define MY_DISPLAY_HEADLESS_H

#include "Config.h"
#include "DisplayBase.h"

class Display : public DisplayBase
{
public:
	Display(ConfigurationBase& config) : DisplayBase() {}

	virtual void begin() {}
	virtual void loop(unsigned long now) {}
	virtual void end() {};

	virtual void publish_telemetry(const String& name, float battery, float temp, float humidity, float pressure, float light) {}
	virtual void publish_name(const String& name) {}
	virtual void publish_status(const String& str) {}
	virtual void log(const String& str) {}

};

#endif
