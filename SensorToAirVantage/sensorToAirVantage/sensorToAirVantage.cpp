/**
 * @file
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"
#include <map>
#include <string>
#include <sstream>

#define BOAT_DEMO

#ifdef BOAT_DEMO

#define KEY_BAROMETER_PRESSURE              "a.bt.read.barometer.pressure"
#define KEY_HUMIDITY_TEMPERATURE           "a.bt.read.humidity.temperature"
#define KEY_HUMIDITY_HUMIDITY              "a.bt.read.humidity.humidity"
#define KEY_BUZZER                         "buzzer.enable"
#define KEY_RED_LED                        "redLED.enable"
#define KEY_GREEN_LED                      "greenLED.enable"

#define KEY_BATTERY_READING                "a.wr.read.battery"
#define KEY_BILGE_STATE                    "a.wr.read.water"
#define KEY_DOCK_POWER                     "a.wr.read.power"
#define KEY_DOOR_STATE                     "a.wr.read.door"
#define KEY_MOTION_DETECT                  "a.wr.read.motion"

#define KEY_GPS_LATITUDE                   "a.gps.location.latitude"
#define KEY_GPS_LONGITUDE                  "a.gps.location.longitude"

#else

#define KEY_BT_IR_TEMPERATURE_AMBIENT      "a.bt.read.ir.ambientTemperature"
#define KEY_BT_IR_TEMPERATURE_IR           "a.bt.read.ir.objectTemperature"
#define KEY_BT_MOVEMENT_GYRO_X             "a.bt.read.motion.gyroscope.x"
#define KEY_BT_MOVEMENT_GYRO_Y             "a.bt.read.motion.gyroscope.y"
#define KEY_BT_MOVEMENT_GYRO_Z             "a.bt.read.motion.gyroscope.z"
#define KEY_BT_MOVEMENT_MAGNETOMETER_X     "a.bt.read.motion.magnetometer.x"
#define KEY_BT_MOVEMENT_MAGNETOMETER_Y     "a.bt.read.motion.magnetometer.y"
#define KEY_BT_MOVEMENT_MAGNETOMETER_Z     "a.bt.read.motion.magnetometer.z"
#define KEY_BT_MOVEMENT_ACCELEROMETER_X    "a.bt.read.motion.accelerometer.x"
#define KEY_BT_MOVEMENT_ACCELEROMETER_Y    "a.bt.read.motion.accelerometer.y"
#define KEY_BT_MOVEMENT_ACCELEROMETER_Z    "a.bt.read.motion.accelerometer.z"
#define KEY_BT_BAROMETR_TEMPERATURE        "a.bt.read.barometer.temperature"
#define KEY_BT_BAROMETR_PRESSURE           "a.bt.read.barometer.pressure"
#define KEY_BT_OPTICAL_LUMINOSITY          "a.bt.read.luminosity"
#define KEY_BT_HUMIDITY_SENSOR_TEMPERATURE "a.bt.read.humidity.temperature"
#define KEY_BT_HUMIDITY_SENSOR_HUMIDITY    "a.bt.read.humidity.humidity"
#define KEY_BT_SHOCK                       "a.bt.read.shock"
#define KEY_BT_ORIENTATION                 "a.bt.read.orientation"
#define KEY_BT_COMPASS                     "a.bt.read.compass"

#define KEY_ARDUINO_TEMPERATURE            "sensors.arduino.temperature"
#define KEY_ARDUINO_HUMIDITY               "sensors.arduino.humidity"
#define KEY_ARDUINO_LUMINOSITY             "sensors.arduino.luminosity"
#define KEY_ARDUINO_NOISE                  "sensors.arduino.noise"
#define KEY_ARDUINO_WATER                  "sensors.arduino.water"
#define KEY_ARDUINO_DUST                   "sensors.arduino.dust"
#define KEY_ARDUINO_OXYGEN                 "sensors.arduino.oxygen"

#define KEY_GPS_LATITUDE                   "sensors.mangoh.gps.latitude"
#define KEY_GPS_LONGITUDE                  "sensors.mangoh.gps.longitude"


#endif   // BOAT_DEMO

#define CFG_KEY_MQTT_HOST     "mqttBrokerHost"
#define CFG_KEY_MQTT_PORT     "mqttBrokerPort"
#define CFG_KEY_MQTT_PASSWORD "mqttBrokerPassword"


class DataValue
{
public:
    DataValue(void);
    explicit DataValue(bool b);
    explicit DataValue(int32_t i);
    explicit DataValue(double f);
    explicit DataValue(const std::string& s);
    DataValue& operator=(const DataValue& other);
    std::string encodeAsJSON(void) const;

private:
    dataRouter_DataType_t type;
    union ValueUnion
    {
        ValueUnion(void) {} // Leave union undefined
        explicit ValueUnion(bool b) : b(b) {}
        explicit ValueUnion(int32_t i) : i(i) {}
        explicit ValueUnion(double f) : f(f) {}
        explicit ValueUnion(const std::string& s) : s(s) {}
        ~ValueUnion(void) {}

        bool b;
        int32_t i;
        double f;
        std::string s;
    } value;
};

// NOTE: commented out some values due to IPC limitations
static const char* pushKeys[] = {
#ifdef BOAT_DEMO
    KEY_BAROMETER_PRESSURE,
    KEY_HUMIDITY_TEMPERATURE,
    KEY_HUMIDITY_HUMIDITY,
    KEY_BUZZER,
    KEY_RED_LED,
    KEY_GREEN_LED,

    KEY_BATTERY_READING,
    KEY_BILGE_STATE,
    KEY_DOCK_POWER,
    KEY_DOOR_STATE,
    KEY_MOTION_DETECT,

    KEY_GPS_LATITUDE,
    KEY_GPS_LONGITUDE,
#else
    KEY_BT_IR_TEMPERATURE_AMBIENT,
    KEY_BT_IR_TEMPERATURE_IR,
    KEY_BT_MOVEMENT_GYRO_X,
    KEY_BT_MOVEMENT_GYRO_Y,
    KEY_BT_MOVEMENT_GYRO_Z,
    KEY_BT_MOVEMENT_MAGNETOMETER_X,
    KEY_BT_MOVEMENT_MAGNETOMETER_Y,
    KEY_BT_MOVEMENT_MAGNETOMETER_Z,
    KEY_BT_MOVEMENT_ACCELEROMETER_X,
    KEY_BT_MOVEMENT_ACCELEROMETER_Y,
    KEY_BT_MOVEMENT_ACCELEROMETER_Z,
    KEY_BT_BAROMETR_TEMPERATURE,
    KEY_BT_BAROMETR_PRESSURE,
    KEY_BT_OPTICAL_LUMINOSITY,
    KEY_BT_HUMIDITY_SENSOR_TEMPERATURE,
    KEY_BT_HUMIDITY_SENSOR_HUMIDITY,
    KEY_BT_SHOCK,
    KEY_BT_ORIENTATION,
    KEY_BT_COMPASS,
    KEY_ARDUINO_TEMPERATURE,
    KEY_ARDUINO_HUMIDITY,
    KEY_ARDUINO_LUMINOSITY,
    KEY_ARDUINO_NOISE,
    KEY_ARDUINO_WATER,
    KEY_ARDUINO_DUST,
    KEY_ARDUINO_OXYGEN,
    KEY_GPS_LATITUDE,
    KEY_GPS_LONGITUDE,
#endif // BOAT_DEMO
};

static struct
{
    le_timer_Ref_t publishTimer;
    std::map<std::string, DataValue> data;
    std::string mqttBrokerPassword;
    std::string publishTopic;
    bool connected;
} globals;

static std::string escapeStringForJSON(const std::string& s)
{
    std::ostringstream jsonStream;
    jsonStream << "\"";
    char tmp[2] = {0};
    for (const char& c: s)
    {
        switch (c)
        {
            case '\\':
                jsonStream << "\\\\";
                break;

            case '\"':
                jsonStream << "\\\"";
                break;

            default:
                tmp[0] = c;
                jsonStream << tmp;
                break;
        }
    }
    jsonStream << "\"";

    return jsonStream.str();
}

static void dataRouterUpdateHandler
(
    dataRouter_DataType_t type,
    const char* key,
    void* contextPtr
)
{
    LE_DEBUG("Processing update for key=%s", key);
    uint32_t timestamp;
    std::string keyStr(key);
    switch (type)
    {
        case DATAROUTER_BOOLEAN:
        {
            bool b;
            dataRouter_ReadBoolean(key, &b, &timestamp);
            DataValue value(b);
            globals.data[keyStr] = value;
            break;
        }

        case DATAROUTER_INTEGER:
        {
            int32_t i;
            dataRouter_ReadInteger(key, &i, &timestamp);
            DataValue value(i);
            globals.data[keyStr] = value;
            break;
        }

        case DATAROUTER_FLOAT:
        {
            double d;
            dataRouter_ReadFloat(key, &d, &timestamp);
            DataValue value(d);
            globals.data[keyStr] = value;
            break;
        }

        case DATAROUTER_STRING:
        {
            char buffer[128];
            dataRouter_ReadString(key, buffer, NUM_ARRAY_MEMBERS(buffer), &timestamp);
            std::string s(buffer);
            DataValue value(s);
            globals.data[keyStr] = value;
            break;
        }

        default:
        {
            LE_FATAL("Invalid data type (%d)", type);
            break;
        }
    }
}

static void publishTimerHandler
(
    le_timer_Ref_t timer
)
{
    if (!globals.connected)
    {
        LE_WARN("Not publishing because MQTT is not connected");
        return;
    }

    std::ostringstream jsonStream;
    jsonStream << "{";
    auto it = globals.data.cbegin();
    while (it != globals.data.cend())
    {
        jsonStream << escapeStringForJSON(it->first) << ":" << it->second.encodeAsJSON();
        it++;
        if (it != globals.data.cend())
        {
            jsonStream << ",";
        }
    }
    jsonStream << "}";

    std::string s = jsonStream.str();
    LE_INFO("About to publish: %s", s.c_str());
    mqtt_Publish(
        globals.publishTopic.c_str(),
        reinterpret_cast<const uint8_t*>(s.c_str()),
        s.length());
}

static void connectMqtt(void)
{
    mqtt_Connect(globals.mqttBrokerPassword.c_str());
}

static void mqttSessionStateHandler
(
    bool isConnected,
    int32_t connectErrorCode,
    int32_t subscriptionErrorCode,
    void* context
)
{
    LE_INFO(
        "Session State: connected=%d, connectErrorCode=%d, subscriptionErrorCode=%d",
        isConnected,
        connectErrorCode,
        subscriptionErrorCode);
    if (isConnected)
    {
        globals.connected = true;
    }
    else
    {
        globals.connected = false;
        // Disconnected, try to reconnect
        //connectMqtt();
    }
}

DataValue::DataValue(void)
{
}

DataValue::DataValue(bool b)
    : type(DATAROUTER_BOOLEAN), value(b)
{
}

DataValue::DataValue(int32_t i)
    : type(DATAROUTER_INTEGER), value(i)
{
}

DataValue::DataValue(double f)
    : type(DATAROUTER_FLOAT), value(f)
{
}

DataValue::DataValue(const std::string& s)
    : type(DATAROUTER_STRING), value(s)
{
}

DataValue& DataValue::operator=(const DataValue& other)
{
    this->type = other.type;
    memcpy(&this->value, &other.value, sizeof(this->value));
    return *this;
}

std::string DataValue::encodeAsJSON(void) const
{
    std::string json;
    switch (this->type)
    {
        case DATAROUTER_BOOLEAN:
            json = (this->value.b) ? "true" : "false";
            break;

        case DATAROUTER_INTEGER:
            json = std::to_string(this->value.i);
            break;

        case DATAROUTER_FLOAT:
            json = std::to_string(this->value.f);
            break;

        case DATAROUTER_STRING:
            json = escapeStringForJSON(this->value.s);
            break;

        default:
            LE_FATAL("Value has unknown type");
            break;
    }
    return json;
}


COMPONENT_INIT
{
    globals.connected = false;

    {
        le_info_ConnectService();
        char imeiBuffer[32];
        LE_ASSERT(le_info_GetImei(imeiBuffer, NUM_ARRAY_MEMBERS(imeiBuffer)) == LE_OK);
        le_info_DisconnectService();
        std::string imei = imeiBuffer;
        globals.publishTopic = imei + "/messages/json";
    }

    le_cfg_IteratorRef_t cfgIter = le_cfg_CreateReadTxn("");
    char mqttBrokerHost[128];
    LE_FATAL_IF(
        le_cfg_GetNodeType(cfgIter, CFG_KEY_MQTT_HOST) != LE_CFG_TYPE_STRING,
        "configTree setting sensorToAirVantage:/mqttBrokerHost either does not exist or is not a "
        "string");
    LE_FATAL_IF(
        le_cfg_GetString(
            cfgIter,
            CFG_KEY_MQTT_HOST,
            mqttBrokerHost,
            NUM_ARRAY_MEMBERS(mqttBrokerHost),
            "") != LE_OK,
        "configTree value sensorToAirVantage:/" CFG_KEY_MQTT_HOST " is too large to fit in the read "
        "buffer");

    LE_FATAL_IF(
        le_cfg_GetNodeType(cfgIter, CFG_KEY_MQTT_PORT) != LE_CFG_TYPE_INT,
        "configTree setting sensorToAirVantage:/" CFG_KEY_MQTT_PORT " either does not exist or is "
        "not an integer");
    const int32_t port = le_cfg_GetInt(cfgIter, CFG_KEY_MQTT_PORT, 0);
    LE_FATAL_IF(
        port <= 0 || port > UINT16_MAX,
        "configTree value sensorToAirVantage:/" CFG_KEY_MQTT_PORT " contains an invalid value (%d)",
        port);

    char mqttBrokerPassword[128];
    LE_FATAL_IF(
        le_cfg_GetNodeType(cfgIter, CFG_KEY_MQTT_PASSWORD) != LE_CFG_TYPE_STRING,
        "configTree setting sensorToAirVantage:/" CFG_KEY_MQTT_PASSWORD " either does not exist "
        "or is not a string");
    LE_FATAL_IF(
        le_cfg_GetString(
            cfgIter,
            CFG_KEY_MQTT_PASSWORD,
            mqttBrokerPassword,
            NUM_ARRAY_MEMBERS(mqttBrokerPassword),
            "") != LE_OK,
        "configTree value sensorToAirVantage:/" CFG_KEY_MQTT_PASSWORD " is too large to fit in "
        "the read buffer");
    le_cfg_CancelTxn(cfgIter);
    globals.mqttBrokerPassword = mqttBrokerPassword;

    const int32_t keepAliveInSeconds = 60;
    const int32_t qos = 0;
    mqtt_Config(mqttBrokerHost, port, keepAliveInSeconds, qos);
    mqtt_AddSessionStateHandler(&mqttSessionStateHandler, NULL);
    connectMqtt();

    // Connect to the data router, but do not set pushAv or the URL and password since this app
    // will be pushing to MQTT directly to work around the limitation that the dataRouter will
    // publish each value individually to MQTT.
    dataRouter_SessionStart("", "", false, DATAROUTER_CACHE);
    for (auto key: pushKeys)
    {
        dataRouter_AddDataUpdateHandler(key, &dataRouterUpdateHandler, NULL);
    }

    globals.publishTimer = le_timer_Create("MQTT push timer");
    LE_ASSERT(globals.publishTimer);
    LE_ASSERT(le_timer_SetHandler(globals.publishTimer, &publishTimerHandler) == LE_OK);
    LE_ASSERT(le_timer_SetMsInterval(globals.publishTimer, 5000) == LE_OK);
    LE_ASSERT(le_timer_SetRepeat(globals.publishTimer, 0) == LE_OK); // repeat forever
    LE_ASSERT(le_timer_Start(globals.publishTimer) == LE_OK);
}

