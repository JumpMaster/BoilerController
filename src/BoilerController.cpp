#define DIAGNOSTIC_PIXEL_PIN  18

#include "StandardFeatures.h"
#include "BoilerController.h"

// Stubs
void mqttCallback(char* topic, byte* payload, unsigned int length);
void publishboilerActive(bool state);

void setBoiler(bool state)
{
    digitalWrite(RELAY_PIN, state);
    publishboilerActive(state);
    boilerActive = state;
    if (state)
        boilerAutoOffTime = millis() + maxBoilerRuntime;
    Log.printf("Boiler switched %s\n", boilerActive ?  "on" : "off");
}

void checkBoilerRelay()
{
    bool state = digitalRead(RELAY_SENSOR_PIN);
    if (state != relaySensorState)
    {
        relaySensorState = state;
        mqttClient.publish(mqttRelaySensor.getStateTopic().c_str(), relaySensorState ? "ON" : "OFF", true);
        diagnosticPixelColor2 = relaySensorState ? pixelBoilerActiveColor : NEOPIXEL_BLACK;
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length)
{

    char data[length + 1];
    memcpy(data, payload, length);
    data[length] = '\0';

    if (strcmp(rebootMQTTButton.getCommandTopic().c_str(), topic) == 0) // Restart Topic
    {
        ESP.restart();
    }
    else if (strcmp(topic, mqttBoilerControlSwitch.getCommandTopic().c_str()) == 0)
    {
        setBoiler(strcmp(data, "ON") == 0);
    }
}

void publishboilerActive(bool state)
{
    mqttClient.publish(mqttBoilerControlSwitch.getStateTopic().c_str(), state ? "ON" : "OFF", true);
}

void checkDeviceConnectionState()
{
    unsigned long currentMillis = millis();

    if (!mqttClient.connected() && boilerMode != BOILER_SAFTEY_MODE)
    {
        if (boilerMode == BOILER_NORMAL)
        {
            boilerMode = BOILER_SAFTEY_MODE_PENDING;
            safteyModeStartTime = millis() + safteyBufferTime;
            Log.println("Boiler mode set to saftey pending");
        }
        else if (boilerMode == BOILER_SAFTEY_MODE_PENDING && millis() > safteyModeStartTime)
        {
            boilerMode = BOILER_SAFTEY_MODE;
            Log.println("Boiler mode set to saftey");
        }
    }
    else if (mqttClient.connected() && boilerMode != BOILER_NORMAL)
    {
        boilerMode = BOILER_NORMAL;
        Log.println("Boiler mode set to normal");
    }

    if (currentMillis > nextboilerActivePublish)
    {
        publishboilerActive(boilerActive);
        nextboilerActivePublish = currentMillis + boilerActivePublishFrequency;
    }

    if (boilerActive && currentMillis > boilerAutoOffTime)
    {
        setBoiler(false);
        Log.println("Boiler switched off due to exceeding max runtime");
    }

    if (boilerMode == BOILER_SAFTEY_MODE && boilerActive == true)
    {
        setBoiler(false);
        Log.println("Boiler switched off due to saftey mode");
    }
}

void manageLocalMQTT()
{
    if (mqttClient.connected() && mqttReconnected)
    {
        mqttReconnected = false;

        mqttClient.publish(rebootMQTTButton.getConfigTopic().c_str(), rebootMQTTButton.getConfigPayload().c_str(), true);
        mqttClient.subscribe(rebootMQTTButton.getCommandTopic().c_str());

        mqttClient.publish(mqttBoilerControlSwitch.getConfigTopic().c_str(), mqttBoilerControlSwitch.getConfigPayload().c_str(), true);
        mqttClient.publish(mqttRelaySensor.getConfigTopic().c_str(), mqttRelaySensor.getConfigPayload().c_str(), true);

        mqttClient.publish(mqttBoilerControlSwitch.getStateTopic().c_str(), relaySensorState ? "ON" : "OFF", true);
        mqttClient.publish(mqttRelaySensor.getStateTopic().c_str(), relaySensorState ? "ON" : "OFF", true);

        mqttClient.subscribe(mqttBoilerControlSwitch.getCommandTopic().c_str());
    }
}

void setup()
{
    pinMode(GPIO_NUM_0, INPUT); // BOOT BUTTON
    
    pinMode(GPIO_NUM_38, OUTPUT); // ONBOARD NEOPIXEL POWER
    digitalWrite(GPIO_NUM_38, LOW); // LOW = OFF, HIGH = ON

    pinMode(RELAY_PIN, OUTPUT); // RELAY
    digitalWrite(RELAY_PIN, LOW);

    pinMode(RELAY_SENSOR_PIN, INPUT); // RELAY SENSOR

    StandardSetup();

    mqttRelaySensor.addConfigVar("device", deviceConfig);
    mqttBoilerControlSwitch.addConfigVar("device", deviceConfig);
    rebootMQTTButton.addConfigVar("device", deviceConfig);

    mqttClient.setCallback(mqttCallback);
}

void loop()
{
    StandardLoop();

    manageLocalMQTT();

    checkDeviceConnectionState();

    checkBoilerRelay();
}