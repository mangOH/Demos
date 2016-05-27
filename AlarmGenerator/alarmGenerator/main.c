#include "legato.h"
#include "interfaces.h"

#define TIME_NOW (time(NULL))

#define ALARM_SHOCK_VAL (100.0)
#define ALARM_TEMPERATURE_VAL (45.0)

// Data to monitor
const char* shockKey            = "shock";
const char* temperatureKey      = "temperature";
const char* orientationKey      = "orientation";

// Data to publish
const char* alarmShockKey       = "alarmshock";
const char* alarmTemperatureKey = "alarmtemperature";
const char* alarmOrientationKey = "alarmorientation";

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

    if (temperatureVal > ALARM_TEMPERATURE_VAL)
    {
        dataRouter_WriteFloat(alarmTemperatureKey, temperatureVal, TIME_NOW);
    }
}

static void OrientationUpdateHandler(
    dataRouter_DataType_t type,
    const char* key,
    void* contextPtr
)
{
    LE_FATAL_IF(
        strcmp(key, orientationKey) != 0,
        "orientation update handler received update for unexpected data key (%s)",
        key);
    LE_FATAL_IF(
        type != DATAROUTER_STRING,
        "orientation update handler received update with unexpected type (%d)",
        type);

    char orientationVal[32];
    uint32_t timestamp;
    dataRouter_ReadString(
        orientationKey, orientationVal, NUM_ARRAY_MEMBERS(orientationVal), &timestamp);

    // TODO: is "1" the orientation of upright?
    if (strcmp("1", orientationVal) != 0)
    {
        dataRouter_WriteString(alarmOrientationKey, orientationVal, TIME_NOW);
    }
}

COMPONENT_INIT
{
    dataRouter_DataUpdateHandlerRef_t shockHandlerRef =
        dataRouter_AddDataUpdateHandler(shockKey, ShockUpdateHandler, NULL);
    dataRouter_DataUpdateHandlerRef_t temperatureHandlerRef =
        dataRouter_AddDataUpdateHandler(temperatureKey, TemperatureUpdateHandler, NULL);
    dataRouter_DataUpdateHandlerRef_t orientationHandlerRef =
        dataRouter_AddDataUpdateHandler(orientationKey, OrientationUpdateHandler, NULL);

    // supress unused variable warnings
    (void)shockHandlerRef;
    (void)temperatureHandlerRef;
    (void)orientationHandlerRef;
}

