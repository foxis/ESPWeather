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
#ifndef MY_DISPLAY_EPAPER_H
#define MY_DISPLAY_EPAPER_H

#include "Config.h"
#include "DisplayBase.h"
#include <GxEPD.h>
#include <GxGDEW0213Z16/GxGDEW0213Z16.cpp>  // 2.13" b/w/r
#include <GxGDEW042T2_FPU/GxGDEW042T2_FPU.cpp>      // 4.2" b/w
#include <GxIO/GxIO_SPI/GxIO_SPI.cpp>
#include <GxIO/GxIO.cpp>

// FreeFonts from Adafruit_GFX
#include <URW_Bookman_L_10.h>
#include <Fonts/FreeMono9pt7b.h>
#include <SPI.h>

#include <map>

#include <icons.h>
#include <imglib/gridicons_video_camera.h>

#define UPDATE_DELAY 5000

class Display : public DisplayBase
{
	// GxIO_SPI(SPIClass& spi, int8_t cs, int8_t dc, int8_t rst = -1, int8_t bl = -1);
	GxIO_SPI io; // arbitrary selection of D3(=0), D4(=2), selected for default of GxEPD_Class
	// GxGDEP015OC1(GxIO& io, uint8_t rst = 12, uint8_t busy = 4);
	GxEPD_Class display; // default selection of D4(=2), D2(=4)

	ConfigurationBase& config;

	typedef struct {
		float battery;
		float temp;
		float humidity;
		float pressure;
	} telemetry_t;

	std::map<String, telemetry_t> telemetry;

public:

	Display(ConfigurationBase& config) :
	 	DisplayBase(),
		config(config),
		io(SPI, 15, 5, 12, -1),
		display(io, 12, 4)
	{
	}

	virtual void begin() {
		display.init();
	  display.setTextColor(GxEPD_BLACK);
		display.setCursor(0, 0);
		display.setRotation(3);
	}

	virtual void end() {
		displayLines();
	}

	void clear()
	{
		display.fillScreen(GxEPD_WHITE);
		display.setRotation(3);
	}

	virtual void publish_telemetry(const String& name, float battery, float temp, float humidity, float pressure)
	{
		telemetry_t tele = {battery, temp, humidity, pressure};

		if (telemetry.find(name) == telemetry.end())
		{
			telemetry.insert(std::pair<String, telemetry_t>(name, tele));
		} else {
			if (tele.battery > 0)
				telemetry[name].battery = tele.battery;
			if (tele.temp > 0)
				telemetry[name].temp = tele.temp;
			if (tele.humidity > 0)
				telemetry[name].humidity = tele.humidity;
			if (tele.pressure > 0)
				telemetry[name].pressure = tele.pressure;
		}
	}

	virtual void publish_name(const String& name)
	{
	}

	virtual void publish_status(const String& str)
	{
	}

	virtual void log(const String& str)
	{
	}

	virtual void loop(unsigned long now)
	{
	}

private:

	void displayLines() {
		char str[21];
		clear();
		display.setCursor(0, 11);
		display.setTextColor(GxEPD_BLACK);
		display.setFont(&FreeMono9pt7b);
		display.println("  Weather Station");

		display.setTextColor(GxEPD_RED);
		display.setFont(&FreeMono9pt7b);

		display.drawBitmap(0, 32, gImage_celcius, 16, 16, GxEPD_RED);
		display.drawBitmap(0, 32 + 18, gImage_gauge, 16, 16, GxEPD_RED);
		display.drawBitmap(0, 32 + 18 * 2, gImage_humidity, 16, 16, GxEPD_RED);
		display.drawBitmap(0, 32 + 18 * 3, gImage_battery, 16, 16, GxEPD_RED);

		std::map<String, telemetry_t>::reverse_iterator I = telemetry.rbegin();
		int index = 0;
		display.setFont(&URW_Bookman_L_Light_10);
		while (I != telemetry.rend()) {
			display.setTextColor(GxEPD_RED);
			display.setCursor(16 + index * 50, 16 + 16 * 1);
			display.print(I->first);
			display.setTextColor(GxEPD_BLACK);
			display.setCursor(16 + index * 50, 16 + 16 * 2);
			sprintf(str, "%.1f", I->second.temp);
			display.print(str);
			display.setCursor(16 + index * 50, 16 + 16 * 3);
			sprintf(str, "%.1f", I->second.pressure);
			display.print(str);
			display.setCursor(16 + index * 50, 16 + 16 * 4);
			sprintf(str, "%.0f", I->second.humidity);
			display.print(str);
			display.setCursor(16 + index * 50, 16 + 16 * 5);
			sprintf(str, "%.2f", I->second.battery);
			display.print(str);
			if (++index > 4)
				break;
			I++;
		}

	  display.update();
	}

};

#endif