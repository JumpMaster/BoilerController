#include <Adafruit_NeoPixel.h>
#include "BoilerControllerESP32.h"
#include <ArduinoOTA.h>
// #include <InternalStorage.h>
// #include <InternalStorageAVR.h>
// #include <InternalStorageESP.h>
// #include <InternalStorageRP2.h>
// #include <InternalStorageSTM32.h>
// #include <OTAStorage.h>
// #include <SDStorage.h>
// #include <SerialFlashStorage.h>
// #include <WiFiOTA.h>

#include <WiFi.h>
// #include <WiFiClient.h>
// #include <WiFiServer.h>
// #include <WiFiUdp.h>

#include <PubSubClient.h>

#include "PapertrailLogger.h"
#include "secrets.h"

const bool debugMode = false;

const gpio_num_t RELAY_PIN = GPIO_NUM_9;
const gpio_num_t RELAY_SENSOR_PIN = GPIO_NUM_17;

// const gpio_num_t NEOPIXEL_PIN = GPIO_NUM_39; // INTERNAL NEOPIXEL
const gpio_num_t NEOPIXEL_PIN = GPIO_NUM_18; // EXTERNAL NEOPIXEL

WiFiClient espClient;
unsigned long wifiReconnectPreviousMillis = 0;
unsigned long wifiReconnectInterval = 30000;

PubSubClient mqttClient(espClient);
unsigned long lastMqttConnectAttempt;
const int mqttConnectAtemptTimeout = 5000;

PapertrailLogger *infoLog;

Adafruit_NeoPixel pixel(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

DeviceConnectionState deviceConnectionState = DEVICE_DISCONNECTED;
BoilerMode boilerMode = BOILER_SAFTEY_MODE;
uint32_t safteyModeStartTime = 0;
const uint32_t safteyBufferTime = 1000*60*10; // 10 minutes
// const uint32_t safteyBufferTime = 1000 * 60 * 2; // 2 minutes
bool boilerActive = 0;
bool relaySensorState = LOW;

// const uint32_t maxBoilerRuntime = 1000 * 60; // 60 seconds
const uint32_t maxBoilerRuntime = 1000 * 60 * 60 * 3; // 3 hours
uint32_t boilerAutoOffTime = 0;

const int boilerActivePublishFrequency = 60000;
uint32_t nextboilerActivePublish = 0;
uint32_t nextBoilerRelayCheck = 0;

bool lightFlashColor = false;
bool brightnessDirection;
uint32_t nextLightUpdate;

uint32_t currentColor = 0xFF0000;
uint32_t stateColor;
const uint32_t boilerActiveColor = 0xFC7B03;

uint8_t currentBrightness = 0;
const uint8_t maxBrightness = 50;

uint32_t nextMetricsUpdate = 0;

// bool bootButtonState = HIGH;
// bool fakeWifi = false;

// Stubs
void mqttCallback(char* topic, byte* payload, unsigned int length);
void publishboilerActive(bool state);

uint32_t getMillis()
{
    return esp_timer_get_time() / 1000;
}

void sendTelegrafMetrics()
{

    uint32_t uptime = getMillis() / 1000;

    char buffer[150];

    snprintf(buffer, sizeof(buffer),
        "status,device=%s uptime=%d,resetReason=%d,firmware=\"%s\",memUsed=%ld,memTotal=%ld",
        deviceName,
        uptime,
        esp_reset_reason(),
        esp_get_idf_version(),
        (ESP.getHeapSize()-ESP.getFreeHeap()),
        ESP.getHeapSize());
    mqttClient.publish("telegraf/particle", buffer);
}

void setBoiler(bool state)
{
    gpio_set_level(RELAY_PIN, state);
    publishboilerActive(state);
    boilerActive = state;
    if (state)
        boilerAutoOffTime = getMillis() + maxBoilerRuntime;
    infoLog->printf("Boiler switched %s\n", boilerActive ?  "on" : "off");
}

void checkBoilerRelay()
{
    bool state = gpio_get_level(RELAY_SENSOR_PIN);
    if (state != relaySensorState)
    {
        relaySensorState = state;
        mqttClient.publish("home/boiler/relay-sensor", relaySensorState ? "ON" : "OFF");
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length)
{

    char data[length + 1];
    memcpy(data, payload, length);
    data[length] = '\0';

    if (strcmp(topic, "home/boiler/switch/set") == 0)
    {
        setBoiler(strcmp(data, "on") == 0);
    }
}

void connectToMQTT()
{
    lastMqttConnectAttempt = millis();
    if (debugMode)
        Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttClient.connect(deviceName, mqtt_usernme, mqtt_password))
    {
        infoLog->println("Connected to MQTT");
        mqttClient.subscribe("home/boiler/switch/set");
    }
}

// TODO make wifi retry instead of within a while not connected loop
void connectToNetwork()
{
    WiFi.begin(ssid, password);
    // WiFi.Scan();
    // delay(5000);
    // WiFi.scanNetworks();

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(5000);
        if (debugMode)
            Serial.println("Establishing connection to WiFi..");
    }

    if (WiFi.status() == WL_CONNECTED)
        infoLog->println("Connected to WiFi");

    if (debugMode)
        Serial.println("Connected to network");
}


void publishboilerActive(bool state)
{
    mqttClient.publish("home/boiler/switch", state ?  "ON" : "OFF");
}

void checkDeviceConnectionState()
{
    DeviceConnectionState cState;

    if (mqttClient.connected())
    {
        cState = DEVICE_MQTT_CONNECTED;
    }
    else if (WiFi.status() == WL_CONNECTED) {
        cState = DEVICE_WIFI_CONNECTED;
    }
    else
    {
        cState = DEVICE_DISCONNECTED;
    }

    if (cState != deviceConnectionState)
    {
        if (cState == DEVICE_DISCONNECTED)
        {
            stateColor = 0xFF0000; // Red
        }
        else if (cState == DEVICE_WIFI_CONNECTED)
        {
            stateColor = 0x0000FF; // Blue
        }
        else if (cState == DEVICE_MQTT_CONNECTED)
        {
            stateColor = 0x00FF00; // Green
        }
    }

    if (cState != DEVICE_MQTT_CONNECTED && boilerMode != BOILER_SAFTEY_MODE)
    {
        if (boilerMode == BOILER_NORMAL)
        {
            boilerMode = BOILER_SAFTEY_MODE_PENDING;
            safteyModeStartTime = getMillis() + safteyBufferTime;
            infoLog->println("Boiler mode set to saftey pending");
        }
        else if (boilerMode == BOILER_SAFTEY_MODE_PENDING && getMillis() > safteyModeStartTime)
        {
            boilerMode = BOILER_SAFTEY_MODE;
            infoLog->println("Boiler mode set to saftey");
        }
    }
    else if (cState == DEVICE_MQTT_CONNECTED && boilerMode != BOILER_NORMAL)
    {
        boilerMode = BOILER_NORMAL;
        infoLog->println("Boiler mode set to normal");
    }

    deviceConnectionState = cState;
}

void updateLed()
{
    if (currentBrightness <= 0)
    {
        brightnessDirection = 1;
        currentColor = stateColor;
        if (boilerActive)
        {
            if (!lightFlashColor)
            {
                currentColor = boilerActiveColor;
            }
            lightFlashColor = !lightFlashColor;
        }
        else if (lightFlashColor)
        {
            lightFlashColor = 0; // Reset this for next time.
        }
    }
    else if (currentBrightness >= maxBrightness)
    {
        brightnessDirection = 0;
    }
    
    currentBrightness = brightnessDirection ? currentBrightness+1 : currentBrightness-1;
    pixel.fill(currentColor);
    pixel.setBrightness(currentBrightness);
    pixel.show();
}

void setup()
{
    gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT); // BOOT BUTTON
    
    gpio_set_direction(GPIO_NUM_39, GPIO_MODE_OUTPUT); // ONBOARD NEOPIXEL POWER
    gpio_set_level(GPIO_NUM_39, LOW); // LOW = OFF, HIGH = ON

    gpio_set_direction(NEOPIXEL_PIN, GPIO_MODE_OUTPUT); // LED
    gpio_set_level(NEOPIXEL_PIN, LOW);

    gpio_set_direction(RELAY_PIN, GPIO_MODE_OUTPUT); // RELAY
    gpio_set_level(RELAY_PIN, LOW);

    gpio_set_direction(RELAY_SENSOR_PIN, GPIO_MODE_INPUT); // RELAY SENSOR

    pixel.begin();
    pixel.fill(0xFF0000);
    pixel.setBrightness(maxBrightness);
    pixel.show();

    // delay(5000);
    // delay(5000);
    // WiFi.disconnect(true,true);
    // WiFi.scanNetworks();

    uint8_t chipid[6];
    esp_read_mac(chipid, ESP_MAC_WIFI_STA);
    char macAddress[19];
    sprintf(macAddress, "%02x:%02x:%02x:%02x:%02x:%02x",chipid[0], chipid[1], chipid[2], chipid[3], chipid[4], chipid[5]);
    infoLog = new PapertrailLogger(papertrailAddress, papertrailPort, LogLevel::Info, macAddress, deviceName);

    connectToNetwork();

    // ArduinoOTA.begin(WiFi.localIP(), "Arduino", "password", InternalStorage);
    ArduinoOTA.setHostname(deviceName);
    ArduinoOTA.begin();

    mqttClient.setServer(mqtt_server, 1883);
    mqttClient.setCallback(mqttCallback);
}

void loop()
{
    unsigned long currentMillis = getMillis();

    ArduinoOTA.handle();

    checkDeviceConnectionState();

    if (currentMillis > nextLightUpdate)
    {    
        nextLightUpdate = millis() + (2000 / maxBrightness);
        updateLed();
    }

    if (boilerActive && currentMillis > boilerAutoOffTime)
    {
        setBoiler(false);
        infoLog->println("Boiler switched off due to exceeding max runtime");
    }

    if (boilerMode == BOILER_SAFTEY_MODE && boilerActive == true)
    {
        setBoiler(false);
        infoLog->println("Boiler switched off due to saftey mode");
    }

    if (WiFi.status() == WL_CONNECTED)
    {

        if (mqttClient.connected())
        {
            mqttClient.loop();

            if (currentMillis > nextMetricsUpdate)
            {
                sendTelegrafMetrics();
                nextMetricsUpdate = currentMillis + 30000;
            }
            
            if (currentMillis > nextboilerActivePublish)
            {
                publishboilerActive(boilerActive);
                nextboilerActivePublish = currentMillis + boilerActivePublishFrequency;
            }

            if (currentMillis > nextBoilerRelayCheck)
            {
                checkBoilerRelay();
                nextBoilerRelayCheck = currentMillis + 1000;
            }

        }
        else if (millis() > (lastMqttConnectAttempt + mqttConnectAtemptTimeout))
        {
            connectToMQTT();
        }

    }
    else if ((WiFi.status() != WL_CONNECTED) && (currentMillis - wifiReconnectPreviousMillis >= wifiReconnectInterval))
    {
        mqttClient.disconnect();
        WiFi.disconnect();
        WiFi.reconnect();

        if (WiFi.status() == WL_CONNECTED)
            infoLog->println("Reconnected to WiFi");

        wifiReconnectPreviousMillis = currentMillis;
    }
/*
    if (bootButtonState != gpio_get_level(GPIO_NUM_0))
    {
        bootButtonState = gpio_get_level(GPIO_NUM_0);
        if (bootButtonState == LOW)
        {
            fakeWifi = !fakeWifi;

            if (fakeWifi == false) 
            {
                connectToNetwork();
            }
            else
            {
                WiFi.begin("FAKEY", "FAKEYFAKEYFAKEFAKE");
            }
        }
        infoLog->printf("Boot button is %s\n", bootButtonState ? "HIGH" : "LOW");
    }
*/
}