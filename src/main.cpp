// PUYA flash is not supported by SPIFFS library
#define PUYA_ISSUE
#include "Config.h"

Configuration config;

void setup() {
	config.begin();
}

void loop() {
	config.loop(millis());
}
