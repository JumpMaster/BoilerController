#pragma once
#ifndef BOILERCONTROLLER_H
#define BOILERCONTROLLER_H

#include "StandardFeatures.h"
#include "HAMqttDevice.h"

typedef enum
{
    BOILER_NORMAL = 0,
    BOILER_SAFTEY_MODE_PENDING = 1,
    BOILER_SAFTEY_MODE = 2
} BoilerMode;

const gpio_num_t RELAY_PIN = GPIO_NUM_9;
const gpio_num_t RELAY_SENSOR_PIN = GPIO_NUM_17;

BoilerMode boilerMode = BOILER_SAFTEY_MODE;
uint32_t safteyModeStartTime = 0;
const uint32_t safteyBufferTime = 1000*60*10; // 10 minutes

bool boilerActive = 0;
bool relaySensorState = LOW;

const uint32_t maxBoilerRuntime = 1000 * 60 * 60 * 5; // 5 hours
uint32_t boilerAutoOffTime = 0;

const int boilerActivePublishFrequency = 60000;
uint32_t nextboilerActivePublish = 0;

const uint32_t pixelBoilerActiveColor = 0xFC7B03;

const char *device_name = "Boiler Controller";
const char *device_id = "a6d8602c-3231-47d3-a6f0-32df9d3a87b9";
const char *device_manufacturer = "Kevin Electronics";
const char *device_hardware = "QT Py ESP32-S2";
const char *device_version = appVersion;

HAMqttParent parentMQTTDevice(device_name,
                              device_id,
                              device_manufacturer,
                              device_hardware,
                              device_version);

HAMqttDevice mqttRelaySensor("Heater Active", "dc5db799-42eb-4a30-a6d2-f57807e5ef30", HAMqttDevice::BINARY_SENSOR);
HAMqttDevice mqttBoilerControlSwitch("Heater", "6b51be29-9045-4b41-9361-b912d38054ce", HAMqttDevice::SWITCH);
HAMqttDevice mqttRebootButton("Reboot Controller", "595a7849-620c-4068-b8a3-71072b812e43", HAMqttDevice::BUTTON);

#endif // BOILERCONTROLLER_H
