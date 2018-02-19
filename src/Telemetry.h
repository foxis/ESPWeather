#ifndef MY_TELEMETRY_H
#define MY_TELEMETRY_H

#include "Config.h"
#include "TelemetryBase.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <BMP280.h>

// These will come from EasyOTA
//#define GETTER(T, name) T name() { return _##name; }
//#define SETTER(T, name) T name(T name) { T pa##name = _##name; _##name = name; return pa##name; }

class Telemetry : public TelemetryBase
{
	DHT_Unified dht11;
	BMP280 bme;
	ConfigurationBase& config;
	unsigned long last_m1;
	bool _init;
	int _skip_readings;

public:
	Telemetry(ConfigurationBase& config) : TelemetryBase(), config(config), dht11(02, DHT11) {

	}

	virtual void begin() {
		dht11.begin();
		bme.begin(SDA, SCL);
		bme.setOversampling(16);

		_battery = 0;
		_temperature = 0;
		_humidity = 0;
		_pressure = 0;
		_skip_readings = 5;
		_init = true;
		_send = false;
		bme_ready = 0;
		last_m1 = millis();
		bme_ready = last_m1 + bme.startMeasurment();
	}

	virtual void loop(unsigned long now) {
		// perform measurements every second
		if (now - last_m1 > 1000) {
			double temp, humi, psi;

			sensors_event_t event;
			dht11.humidity().getEvent(&event);
			humi = event.relative_humidity;

			for (int i = 0; i < 10; i++)
				_battery += analogRead(A0) / 1024.0;

			_humidity += humi;

			// start BMP280 measurements
			if (bme_ready == 0) {
				bme_ready = now + bme.startMeasurment();
			}

			// Read BMP280 measurements
			// NOTE: Will perform humidity and battery voltage measurements up til now
			if (now - bme_ready > 0) {
				bme.getTemperatureAndPressure(temp, psi);
				if (_skip_readings)
				{
					_send = false;
					_skip_readings--;
				} else if (_temperature == 0) {
					_pressure = psi * 2;
					_temperature = temp * 2;
					_send = true;
				} else {
					_pressure += psi;
					_temperature += temp;
					_send = true;
				}
				bme_ready = 0;
				last_m1 = now;
			} else {
				// retain messages until true measurement comes in
				_temperature *= 2;
				_pressure *= 2;
			}

			// first time measurements
			if (!_init)  {
				_pressure /= 2.0;
				_temperature /= 2.0;
				_humidity /= 2.0;
				_battery /= 2.0;
			} else {
				_init = false;
			}
		}
	}
};

#endif
