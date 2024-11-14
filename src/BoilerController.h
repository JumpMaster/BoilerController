#pragma once
#ifndef BOILERCONTROLLER_H
#define BOILERCONTROLLER_H

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

const char* deviceConfig = "{\"identifiers\":\"195212a9-76d2-4b3e-8d33-78c5f4e7689a\",\"name\":\"Boiler Controller\",\"sw_version\":\"2024.11.1\",\"model\":\"BoilerController\",\"manufacturer\":\"JumpMaster\"}";
HAMqttDevice mqttRelaySensor("Boiler Active Sensor", HAMqttDevice::BINARY_SENSOR);
HAMqttDevice mqttBoilerControlSwitch("Boiler Active", HAMqttDevice::SWITCH);
HAMqttDevice rebootMQTTButton("Boiler Controller Reboot", HAMqttDevice::BUTTON);

#endif // BOILERCONTROLLER_H
