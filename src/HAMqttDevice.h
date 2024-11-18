#ifndef HA_MQTT_DEVICE_H
#define HA_MQTT_DEVICE_H

#include "Arduino.h"
#include <vector>

class HAMqttDevice
{
public:
    enum DeviceType
    {
        ALARM_CONTROL_PANEL,
        BINARY_SENSOR,
        CAMERA,
        COVER,
        FAN,
        LIGHT,
        LOCK,
        SENSOR,
        SWITCH,
        CLIMATE,
        VACUUM,
        NUMBER,
        BUTTON
    };

private:
    // Device proprieties
    const String _name;
    const String _unique_id;
    const DeviceType _type;
    const String haMQTTPrefix = "homeassistant";

    String _identifier;
    String _topic;
    String _parent_identifier;

    // Config variables handling
    struct ConfigVar
    {
        String key;
        String value;
    };
    std::vector<ConfigVar> _configVars;

    // Device attributes handling
    struct Attribute
    {
        String key;
        String value;
    };
    std::vector<Attribute> _attributes;

public:
    HAMqttDevice(
        const String &name,
        const String &unique_id,
        const DeviceType type);

    ~HAMqttDevice();

    HAMqttDevice &enableCommandTopic();
    HAMqttDevice &enableStateTopic();
    HAMqttDevice &enableAttributesTopic();
    HAMqttDevice &UpdateTopic();
    HAMqttDevice &SetParentDeviceIdentifier(const String &parent_identifier);

    HAMqttDevice &addConfigVar(const String &key, const String &value);
    HAMqttDevice &addAttribute(const String &key, const String &value);
    HAMqttDevice &clearAttributes();

    const String getConfigPayload() const;
    const String getAttributesPayload() const;
    const String getDeviceConfigPayload() const;

    inline const String getTopic() const { return _topic; }
    inline const String getStateTopic() const { return _topic + "/state"; }
    inline const String getConfigTopic() const { return _topic + "/config"; }
    inline const String getAttributesTopic() const { return _topic + "/attr"; }
    inline const String getCommandTopic() const { return _topic + "/cmd"; }

private:
    static String deviceTypeToStr(DeviceType type);
};


class HAMqttParent
{
private:
    // Device proprieties
    const String _device_name;
    const String _device_unique_id;
    const String _device_hardware;
    const String _device_manufacturer;
    const String _device_version;
    const String haMQTTPrefix = "homeassistant";

    String _device_identifier;
    String _device_topic;
    std::vector<HAMqttDevice *> haMqttDevices;

public:
    HAMqttParent(
        const String &name,
        const String &unique_id,
        const String &manufacturer,
        const String &hardware,
        const String &version);

    ~HAMqttParent();

    void addHAMqttDevice(HAMqttDevice *childDevice);
    const String getConfigPayload() const;
    inline const String getConfigTopic() const { return _device_topic + "/config"; }
};

#endif