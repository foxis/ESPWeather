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

	virtual void publish_telemetry(const String& name)
	{
		char str[21];
		sprintf(str, "T: %.1f", config.telemetry._temperature);
		fillLine(str, 1);
		sprintf(str, "P: %.1f", config.telemetry._pressure);
		fillLine(str, 2);
		sprintf(str, "H: %.1f", config.telemetry._humidity);
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

	virtual void loop(long now)
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
