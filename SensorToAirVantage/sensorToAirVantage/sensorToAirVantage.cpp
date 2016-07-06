#include "legato.h"
#include "interfaces.h"
#include <map>
#include <string>
#include <sstream>

#define KEY_IR_TEMPERATURE_AMBIENT      "sensors.bluetooth.ir.ambientTemperature"
#define KEY_IR_TEMPERATURE_IR           "sensors.bluetooth.ir.objectTemperature"
#define KEY_MOVEMENT_GYRO_X             "sensors.bluetooth.motion.gyroscope.x"
#define KEY_MOVEMENT_GYRO_Y             "sensors.bluetooth.motion.gyroscope.y"
#define KEY_MOVEMENT_GYRO_Z             "sensors.bluetooth.motion.gyroscope.z"
#define KEY_MOVEMENT_MAGNETOMETER_X     "sensors.bluetooth.motion.magnetometer.x"
#define KEY_MOVEMENT_MAGNETOMETER_Y     "sensors.bluetooth.motion.magnetometer.y"
#define KEY_MOVEMENT_MAGNETOMETER_Z     "sensors.bluetooth.motion.magnetometer.z"
#define KEY_MOVEMENT_ACCELEROMETER_X    "sensors.bluetooth.motion.accelerometer.x"
#define KEY_MOVEMENT_ACCELEROMETER_Y    "sensors.bluetooth.motion.accelerometer.y"
#define KEY_MOVEMENT_ACCELEROMETER_Z    "sensors.bluetooth.motion.accelerometer.z"
#define KEY_BAROMETR_TEMPERATURE        "sensors.bluetooth.barometer.temperature"
#define KEY_BAROMETR_PRESSURE           "sensors.bluetooth.barometer.pressure"
#define KEY_OPTICAL_LUMINOSITY          "sensors.bluetooth.luminosity"
#define KEY_HUMIDITY_SENSOR_TEMPERATURE "sensors.bluetooth.humidity.temperature"
#define KEY_HUMIDITY_SENSOR_HUMIDITY    "sensors.bluetooth.humidity.humidity"
#define KEY_SHOCK                       "sensors.bluetooth.shock"
#define KEY_ORIENTATION                 "sensors.bluetooth.orientation"
#define KEY_COMPASS                     "sensors.bluetooth.compass"

#define CFG_KEY_MQTT_HOST     "mqttBrokerHost"
#define CFG_KEY_MQTT_PORT     "mqttBrokerPort"
#define CFG_KEY_MQTT_PASSWORD "mqttBrokerPassword"


class DataValue
{
public:
    explicit DataValue(bool b);
    explicit DataValue(int32_t i);
    explicit DataValue(double f);
    explicit DataValue(const std::string& s);
    ~DataValue(void);
    DataValue(const DataValue& other);
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

static const char* pushKeys[] = {
    KEY_IR_TEMPERATURE_AMBIENT,
    KEY_IR_TEMPERATURE_IR,
    KEY_MOVEMENT_GYRO_X,
    KEY_MOVEMENT_GYRO_Y,
    KEY_MOVEMENT_GYRO_Z,
    KEY_MOVEMENT_MAGNETOMETER_X,
    KEY_MOVEMENT_MAGNETOMETER_Y,
    KEY_MOVEMENT_MAGNETOMETER_Z,
    KEY_MOVEMENT_ACCELEROMETER_X,
    KEY_MOVEMENT_ACCELEROMETER_Y,
    KEY_MOVEMENT_ACCELEROMETER_Z,
    KEY_BAROMETR_TEMPERATURE,
    KEY_BAROMETR_PRESSURE,
    KEY_OPTICAL_LUMINOSITY,
    KEY_HUMIDITY_SENSOR_TEMPERATURE,
    KEY_HUMIDITY_SENSOR_HUMIDITY,
    KEY_SHOCK,
    KEY_ORIENTATION,
    KEY_COMPASS,
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
            globals.data.insert(std::make_pair(keyStr, value));
            break;
        }

        case DATAROUTER_INTEGER:
        {
            int32_t i;
            dataRouter_ReadInteger(key, &i, &timestamp);
            DataValue value(i);
            globals.data.insert(std::make_pair(keyStr, value));
            break;
        }

        case DATAROUTER_FLOAT:
        {
            double d;
            dataRouter_ReadFloat(key, &d, &timestamp);
            DataValue value(d);
            globals.data.insert(std::make_pair(keyStr, value));
            break;
        }

        case DATAROUTER_STRING:
        {
            char buffer[128];
            dataRouter_ReadString(key, buffer, NUM_ARRAY_MEMBERS(buffer), &timestamp);
            std::string s(buffer);
            DataValue value(s);
            globals.data.insert(std::make_pair(keyStr, value));
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

DataValue::DataValue(const DataValue& other)
    : type(other.type)
{
    memcpy(&this->value, &other.value, sizeof(this->value));
}

DataValue::~DataValue(void)
{
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

