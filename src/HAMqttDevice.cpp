#include "HAMqttDevice.h"

HAMqttDevice::HAMqttDevice(
    const String &name,
    const String &unique_id,
    const DeviceType type) : _name(name),
                             _unique_id(unique_id),
                             _type(type)
{
    // Id = name to lower case replacing spaces by underscore (ex: name="Kitchen Light" -> id="kitchen_light")
    _identifier = name;
    _identifier.replace(' ', '_');
    _identifier.toLowerCase();

    // Define the MQTT topic of the device
    UpdateTopic();

    // Preconfigure mandatory config vars that we already know
    addConfigVar("~", _topic);
    addConfigVar("name", _name);
    addConfigVar("unique_id", unique_id);

    // When the command topic is mandatory, enable it.
    switch (type)
    {
    case DeviceType::ALARM_CONTROL_PANEL:
    case DeviceType::FAN:
    case DeviceType::LIGHT:
    case DeviceType::LOCK:
    case DeviceType::COVER:
    case DeviceType::NUMBER:
    case DeviceType::SWITCH:
    case DeviceType::BUTTON:
        enableCommandTopic();
    default:
        break;
    }

    // When the state topic is mandatory, enable it.
    switch (type)
    {
    case DeviceType::ALARM_CONTROL_PANEL:
    case DeviceType::BINARY_SENSOR:
    case DeviceType::FAN:
    case DeviceType::LIGHT:
    case DeviceType::LOCK:
    case DeviceType::COVER:
    case DeviceType::NUMBER:
    case DeviceType::SENSOR:
    case DeviceType::SWITCH:
        enableStateTopic();
    default:
        break;
    }
}

HAMqttDevice::~HAMqttDevice() {}

HAMqttDevice &HAMqttDevice::UpdateTopic()
{
    _topic = haMQTTPrefix + '/' + deviceTypeToStr(_type) + '/';
    
    if (_parent_identifier.length() > 0)
    {
        _topic.concat(_parent_identifier + '/');
    }
    
    _topic.concat(_identifier);

    for (uint8_t i = 0; i < _configVars.size(); i++)
    {
        if (_configVars[i].key[0] == '~')
        {
            _configVars[i].value = _topic;
            break;
        }
    }

    return *this;
}

HAMqttDevice &HAMqttDevice::SetParentDeviceIdentifier(const String &parent_identifier)
{
    _parent_identifier = parent_identifier;
    UpdateTopic();
    return *this;
}

HAMqttDevice &HAMqttDevice::enableCommandTopic()
{
    addConfigVar("cmd_t", "~/cmd");
    return *this;
}

HAMqttDevice &HAMqttDevice::enableStateTopic()
{
    addConfigVar("stat_t", "~/state");
    return *this;
}

HAMqttDevice &HAMqttDevice::enableAttributesTopic()
{
    addConfigVar("json_attr_t", "~/attr");
    return *this;
}

HAMqttDevice &HAMqttDevice::addConfigVar(const String &name, const String &value)
{
    _configVars.push_back({name, value});
    return *this;
}

HAMqttDevice &HAMqttDevice::addAttribute(const String &name, const String &value)
{
    _attributes.push_back({name, value});
    return *this;
}

HAMqttDevice &HAMqttDevice::clearAttributes()
{
    _attributes.clear();
    return *this;
}

const String HAMqttDevice::getConfigPayload() const
{
    String configPayload = "{";

    for (uint8_t i = 0; i < _configVars.size(); i++)
    {
        configPayload.concat('"');
        configPayload.concat(_configVars[i].key);
        configPayload.concat("\":");

        bool valueIsDictionnary = _configVars[i].value[0] == '{';

        if (!valueIsDictionnary)
            configPayload.concat('"');

        configPayload.concat(_configVars[i].value);

        if (!valueIsDictionnary)
            configPayload.concat('"');

        configPayload.concat(',');
    }
    configPayload.setCharAt(configPayload.length() - 1, '}');

    return configPayload;
}

const String HAMqttDevice::getDeviceConfigPayload() const
{
    String deviceConfigPayload;

    deviceConfigPayload.concat("\"" + _identifier + "\":{\"p\":\"");
    deviceConfigPayload.concat(deviceTypeToStr(_type));
    deviceConfigPayload.concat("\",");

    for (uint8_t i = 0; i < _configVars.size(); i++)
    {
        if (i != 0)
            deviceConfigPayload.concat(',');

        deviceConfigPayload.concat('"');
        deviceConfigPayload.concat(_configVars[i].key);
        deviceConfigPayload.concat("\":");

        bool valueIsDictionnary = _configVars[i].value[0] == '{' || _configVars[i].value[0] == '[';

        if (!valueIsDictionnary)
            deviceConfigPayload.concat('"');

        deviceConfigPayload.concat(_configVars[i].value);

        if (!valueIsDictionnary)
            deviceConfigPayload.concat('"');
    }
    deviceConfigPayload.concat("}");
    return deviceConfigPayload;
}

const String HAMqttDevice::getAttributesPayload() const
{
    String attrPayload = "{";

    for (uint8_t i = 0; i < _attributes.size(); i++)
    {
        attrPayload.concat('"');
        attrPayload.concat(_attributes[i].key);
        attrPayload.concat("\":\"");
        attrPayload.concat(_attributes[i].value);
        attrPayload.concat("\",");
    }
    attrPayload.setCharAt(attrPayload.length() - 1, '}');

    return attrPayload;
}

String HAMqttDevice::deviceTypeToStr(DeviceType type)
{
    switch (type)
    {
    case DeviceType::ALARM_CONTROL_PANEL:
        return "alarm_control_panel";
    case DeviceType::BINARY_SENSOR:
        return "binary_sensor";
    case DeviceType::CAMERA:
        return "camera";
    case DeviceType::COVER:
        return "cover";
    case DeviceType::FAN:
        return "fan";
    case DeviceType::LIGHT:
        return "light";
    case DeviceType::LOCK:
        return "lock";
    case DeviceType::SENSOR:
        return "sensor";
    case DeviceType::SWITCH:
        return "switch";
    case DeviceType::CLIMATE:
        return "climate";
    case DeviceType::VACUUM:
        return "vacuum";
    case DeviceType::NUMBER:
        return "number";
    case DeviceType::BUTTON:
        return "button";
    default:
        return "[Unknown DeviceType]";
    }
}


HAMqttParent::HAMqttParent(
    const String &name,
    const String &unique_id,
    const String &manufacturer,
    const String &hardware,
    const String &version) : _device_name(name),
                             _device_unique_id(unique_id),
                             _device_manufacturer(manufacturer),
                             _device_hardware(hardware),
                             _device_version(version)
{
    _device_identifier = _device_name;
    _device_identifier.replace(' ', '_');
    _device_identifier.toLowerCase();

    _device_topic = haMQTTPrefix + "/device/" + _device_identifier;

}

HAMqttParent::~HAMqttParent() {}

void HAMqttParent::addHAMqttDevice(HAMqttDevice *childDevice)
{   
    haMqttDevices.push_back(childDevice);
    
    childDevice->SetParentDeviceIdentifier(_device_identifier);
}

const String HAMqttParent::getConfigPayload() const
{
    String parentDeviceConfig = "{\"dev\":{\"ids\":\"";
    parentDeviceConfig.concat(_device_unique_id);
    parentDeviceConfig.concat("\",\"name\":\"");
    parentDeviceConfig.concat(_device_name);
    parentDeviceConfig.concat("\",\"mf\":\"");
    parentDeviceConfig.concat(_device_manufacturer);
    parentDeviceConfig.concat("\",\"mdl\":\"");
    parentDeviceConfig.concat(_device_name);
    parentDeviceConfig.concat("\",\"sw\":\"");
    parentDeviceConfig.concat(_device_version);
    parentDeviceConfig.concat("\",\"hw\":\"");
    parentDeviceConfig.concat(_device_hardware);
    parentDeviceConfig.concat("\"},\"o\":{\"name\":\"");
    parentDeviceConfig.concat("HAMqttParent");
    parentDeviceConfig.concat("\"},\"cmps\":{");

    for (uint8_t i = 0; i < haMqttDevices.size(); i++)
    {
        if (i != 0)
            parentDeviceConfig.concat(",");

        parentDeviceConfig.concat(haMqttDevices[i]->getDeviceConfigPayload());
    }

    parentDeviceConfig.concat("}}");

    return parentDeviceConfig;
}
