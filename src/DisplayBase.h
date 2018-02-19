#ifndef MY_DISPLAY_BASE_H
#define MY_DISPLAY_BASE_H

class DisplayBase
{
public:
	virtual void begin() = 0;
	virtual void end() = 0;
	virtual void loop(unsigned long now) = 0;

	virtual void publish_telemetry(const String& name, float battery, float temp, float humidity, float pressure) = 0;
	virtual void publish_name(const String& name) = 0;
	virtual void publish_status(const String& str) = 0;
	virtual void log(const String& str) = 0;
};

#endif
