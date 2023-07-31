#pragma once
#ifndef BOILERCONTROLLER_H
#define BOILERCONTROLLER_H

#include "HAMqttDevice.h"
/*
    typedef enum {
        DEVICE_DISCONNECTED = 0,
        DEVICE_WIFI_CONNECTED = 1,
        DEVICE_MQTT_CONNECTED = 2
    } DeviceConnectionState;
*/
    typedef enum {
        BOILER_NORMAL = 0,
        BOILER_SAFTEY_MODE_PENDING = 1,
        BOILER_SAFTEY_MODE = 2
    } BoilerMode;

#endif // BOILERCONTROLLER_H
