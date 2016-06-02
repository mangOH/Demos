#include "legato.h"
#include "interfaces.h"

#define TIME_NOW (time(NULL))

#define ALARM_SHOCK_VAL           (1.0)
#define ALARM_TEMPERATURE_ON_VAL  (31.0)
#define ALARM_TEMPERATURE_OFF_VAL (30.0)
#define ALARM_HUMIDITY_ON_VAL     (45.0)
#define ALARM_HUMIDITY_OFF_VAL    (40.0)
#define ALARM_LUMINOSITY_ON_VAL   (3000.0)
#define ALARM_LUMINOSITY_OFF_VAL  (1000.0)

// Data to monitor
const char* shockKey            = "container.shock";
const char* temperatureKey      = "container.temperature";
const char* humidityKey         = "container.humidity";
const char* luminosityKey       = "container.luminosity";
const char* orientationKey      = "container.orientation";

// Data to publish
const char* alarmShockKey       = "container.alarmShock";
const char* alarmTemperatureKey = "container.alarmTemperature";
const char* alarmHumidityKey    = "container.alarmHumidity";
const char* alarmLuminosityKey  = "container.alarmLuminosity";
const char* alarmOrientationKey = "container.alarmOrientation";

static void ShockUpdateHandler(
    dataRouter_DataType_t type,
    const char* key,
    void* contextPtr
)
{
    LE_FATAL_IF(
        strcmp(key, shockKey) != 0,
        "shock update handler received update for unexpected data key (%s)",
        key);
    LE_FATAL_IF(
        type != DATAROUTER_FLOAT,
        "shock update handler received update with unexpected type (%d)",
        type);

    double shockVal;
    uint32_t timestamp;
    dataRouter_ReadFloat(shockKey, &shockVal, &timestamp);

    if (shockVal > ALARM_SHOCK_VAL)
    {
        dataRouter_WriteFloat(alarmShockKey, shockVal, TIME_NOW);
    }
}

static void TemperatureUpdateHandler(
    dataRouter_DataType_t type,
    const char* key,
    void* contextPtr
)
{
    static bool alarmOn = false;
    static bool anyUpdateReceived = false;
    LE_FATAL_IF(
        strcmp(key, temperatureKey) != 0,
        "temperature update handler received update for unexpected data key (%s)",
        key);
    LE_FATAL_IF(
        type != DATAROUTER_FLOAT,
        "temperature update handler received update with unexpected type (%d)",
        type);

    double temperatureVal;
    uint32_t timestamp;
    dataRouter_ReadFloat(temperatureKey, &temperatureVal, &timestamp);

    if ((!alarmOn || !anyUpdateReceived) && temperatureVal > ALARM_TEMPERATURE_ON_VAL)
    {
        alarmOn = true;
        dataRouter_WriteBoolean(alarmTemperatureKey, alarmOn, TIME_NOW);
    }
    else if (alarmOn && temperatureVal < ALARM_TEMPERATURE_OFF_VAL)
    {
        alarmOn = false;
        dataRouter_WriteBoolean(alarmTemperatureKey, alarmOn, TIME_NOW);
    }
    anyUpdateReceived = true;
}

static void HumidityUpdateHandler(
    dataRouter_DataType_t type,
    const char* key,
    void* contextPtr
)
{
    static bool alarmOn = false;
    static bool anyUpdateReceived = false;
    LE_FATAL_IF(
        strcmp(key, humidityKey) != 0,
        "humidity update handler received update for unexpected data key (%s)",
        key);
    LE_FATAL_IF(
        type != DATAROUTER_FLOAT,
        "humidity update handler received update with unexpected type (%d)",
        type);

    double humidityVal;
    uint32_t timestamp;
    dataRouter_ReadFloat(humidityKey, &humidityVal, &timestamp);

    if ((!alarmOn || !anyUpdateReceived) && humidityVal > ALARM_HUMIDITY_ON_VAL)
    {
        alarmOn = true;
        dataRouter_WriteBoolean(alarmHumidityKey, alarmOn, TIME_NOW);
    }
    else if (alarmOn && humidityVal < ALARM_HUMIDITY_OFF_VAL)
    {
        alarmOn = false;
        dataRouter_WriteBoolean(alarmHumidityKey, alarmOn, TIME_NOW);
    }
    anyUpdateReceived = true;
}

static void LuminosityUpdateHandler(
    dataRouter_DataType_t type,
    const char* key,
    void* contextPtr
)
{
    static bool alarmOn = false;
    static bool anyUpdateReceived = false;
    LE_FATAL_IF(
        strcmp(key, luminosityKey) != 0,
        "luminosity update handler received update for unexpected data key (%s)",
        key);
    LE_FATAL_IF(
        type != DATAROUTER_FLOAT,
        "luminosity update handler received update with unexpected type (%d)",
        type);

    double luminosityVal;
    uint32_t timestamp;
    dataRouter_ReadFloat(luminosityKey, &luminosityVal, &timestamp);

    if ((!alarmOn || !anyUpdateReceived) && luminosityVal > ALARM_LUMINOSITY_ON_VAL)
    {
        alarmOn = true;
        dataRouter_WriteBoolean(alarmLuminosityKey, alarmOn, TIME_NOW);
    }
    else if (alarmOn && luminosityVal < ALARM_LUMINOSITY_OFF_VAL)
    {
        alarmOn = false;
        dataRouter_WriteBoolean(alarmLuminosityKey, alarmOn, TIME_NOW);
    }
    anyUpdateReceived = true;
}

static void OrientationUpdateHandler(
    dataRouter_DataType_t type,
    const char* key,
    void* contextPtr
)
{
    static bool alarmOn = false;
    static bool anyUpdateReceived = false;
    LE_FATAL_IF(
        strcmp(key, orientationKey) != 0,
        "orientation update handler received update for unexpected data key (%s)",
        key);
    LE_FATAL_IF(
        type != DATAROUTER_STRING,
        "orientation update handler received update with unexpected type (%d)",
        type);

    int32_t orientationVal;
    uint32_t timestamp;
    dataRouter_ReadInteger(orientationKey, &orientationVal, &timestamp);

    const int32_t upright = 3;
    bool newAlarmOn = orientationVal != upright;
    if (!anyUpdateReceived || newAlarmOn != alarmOn)
    {
        alarmOn = newAlarmOn;
        anyUpdateReceived = true;
        dataRouter_WriteBoolean(alarmOrientationKey, newAlarmOn, TIME_NOW);
    }
}

COMPONENT_INIT
{
    dataRouter_SessionStart("eu.airvantage.net", "SwiBridge", true, DATAROUTER_CACHE);
    dataRouter_DataUpdateHandlerRef_t shockHandlerRef =
        dataRouter_AddDataUpdateHandler(shockKey, ShockUpdateHandler, NULL);
    dataRouter_DataUpdateHandlerRef_t temperatureHandlerRef =
        dataRouter_AddDataUpdateHandler(temperatureKey, TemperatureUpdateHandler, NULL);
    dataRouter_DataUpdateHandlerRef_t humidityHandlerRef =
        dataRouter_AddDataUpdateHandler(humidityKey, HumidityUpdateHandler, NULL);
    dataRouter_DataUpdateHandlerRef_t luminosityHandlerRef =
        dataRouter_AddDataUpdateHandler(luminosityKey, LuminosityUpdateHandler, NULL);
    dataRouter_DataUpdateHandlerRef_t orientationHandlerRef =
        dataRouter_AddDataUpdateHandler(orientationKey, OrientationUpdateHandler, NULL);

    // supress unused variable warnings
    (void)shockHandlerRef;
    (void)temperatureHandlerRef;
    (void)humidityHandlerRef;
    (void)luminosityHandlerRef;
    (void)orientationHandlerRef;
}

