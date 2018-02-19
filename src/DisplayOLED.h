/**

MIT License

Copyright (c) 2018 foxis (Andrius Mikonis <andrius.mikonis@gmail.com>)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

 **/
#ifndef MY_DISPLAY_OLED_H
#define MY_DISPLAY_OLED_H

#include "Config.h"
#include "DisplayBase.h"
#include <SSD1306.h>

#define LINES 6
#define LINE_LENGTH 21

class Display : public DisplayBase
{
	SSD1306 display;
	ConfigurationBase& config;
	char _lines[LINES][LINE_LENGTH + 1] = {"Weather Station", "", "", ""};
public:

	Display(ConfigurationBase& config) : DisplayBase(), config(config), display(0x3c, SDA, SCL) {

	}

	virtual void begin() {
		display.init();
	  display.clear();
	  display.setFont(ArialMT_Plain_10);
	  display.setColor(WHITE);
	}
	virtual void end() {
		publish_status("");
	};

	virtual void publish_telemetry(const String& name, float battery, float temp, float humidity, float pressure)
	{
		char str[21];
		sprintf(str, "T: %.1f", temp);
		fillLine(str, 1);
		sprintf(str, "P: %.1f", pressure);
		fillLine(str, 2);
		sprintf(str, "H: %.1f", humidity);
		fillLine(str, 3);
		displayLines();
	}

	virtual void publish_name(const String& name)
	{
		displayLine(name.c_str(), 0);
	}

	virtual void publish_status(const String& str)
	{
		displayLine(str.c_str(), 0);
	}

	virtual void log(const String& str)
	{
		displayLine(str.c_str(), 4);
	}

	virtual void loop(unsigned long now)
	{

	}

private:
	void fillLine(const char *message, int line_nr)
	{
		snprintf(_lines[line_nr + 1], LINE_LENGTH, "%s", message);
	}

	void displayLines() {
		display.clear();
		for (int i = 0; i < LINES; i++)
	  	display.drawString(0, 10 * i, _lines[i]);
	  display.display();
	}

	void displayLine(const char * message, int line_nr) {
		fillLine(message, line_nr);
		displayLines();
	}
};

#endif
