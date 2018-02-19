#ifndef MY_TELEMETRY_BASE_H
#define MY_TELEMETRY_BASE_H

class TelemetryBase
{
public:
	TelemetryBase() {

	}

	virtual void begin() = 0;
	virtual void loop(unsigned long now) = 0;

	float _battery, _temperature, _humidity, _pressure;
	bool _send;
	long bme_ready;
};

#endif
