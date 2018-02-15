# ESPWeather
ESP-1S or ESP-12 weather station

# Topics being published by the device

## announce

Device will publish it's name to this topic. Initially it will be a MAC address of the ESP.
One can configure a different name by publishing to `{device name}/name` a new name which will be saved on the device.

## {device name}/temperature

Temperature of the surroundings in celcius.

## {device name}/pressure

Air pressure of the surroundings in kPa.

## {device name}/humidity

Air relative humidity in percentage.

## {device name}/battery

Battery voltage in volts.

# Topics being subscribed by the Device

## {device name}/name

Publishing to this topic will change device name.

## {device name}/apadd

Publishing to this topic will add another Wifi Network.
One must supply a space delimited ssid and password, e.g. `ssid password`.
This will be saved to the device and it will try to connect to this and other saved APs on boot.

## {device name}/apremove

Publishing to this topic will remove the ap. One must publish ssid of the network that one wishes to remove.

## {device name}

This topic accepts following publishes:

### SLEEP

Forces the device to sleep for preset time (around 30 minutes).

### NOSLEEP

Disables sleeping of the devices. This does not percist after restart.
Useful for OTA development/etc.

### RESTART

Forces restart of the device.

### PING

Forces to announce it's name on `announce` topic.
Useful for device status monitoring.


# Things to note

Until now (20180216) ESP-01 with PUYA flash chips are not supported by the SPIFFS library (it can read uploaded, but not properly write files.).
As a workaround one can publish config topics with persistent messages. E.g. setting name of the device. Setting up Wifi in this fashion isn't really useful for obvious reasons though.
