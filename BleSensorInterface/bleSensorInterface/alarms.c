#include "legato.h"
#include "interfaces.h"

#define TIME_NOW (time(NULL))

#define ALARM_SHOCK_VAL           (1.0)
#define ALARM_TEMPERATURE_ON_VAL  (31.0)
#define ALARM_TEMPERATURE_OFF_VAL (30.0)
#define ALARM_HUMIDITY_ON_VAL     (45.0)
#define ALARM_HUMIDITY_OFF_VAL    (40.0)
#define ALARM_LUMINOSITY_ON_VAL   (50.0)
#define ALARM_LUMINOSITY_OFF_VAL  (40.0)

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

void checkShockAlarm(double shockValue)
{
    if (shockValue > ALARM_SHOCK_VAL)
    {
        dataRouter_WriteFloat(alarmShockKey, shockValue, TIME_NOW);
    }
}

void checkTemperatureAlarm(double temperature)
{
    static bool alarmOn = false;
    static bool anyUpdateReceived = false;

    if ((!alarmOn || !anyUpdateReceived) && temperature > ALARM_TEMPERATURE_ON_VAL)
    {
        alarmOn = true;
        dataRouter_WriteBoolean(alarmTemperatureKey, alarmOn, TIME_NOW);
    }
    else if (alarmOn && temperature < ALARM_TEMPERATURE_OFF_VAL)
    {
        alarmOn = false;
        dataRouter_WriteBoolean(alarmTemperatureKey, alarmOn, TIME_NOW);
    }
    anyUpdateReceived = true;
}

void checkHumidityAlarm(double humidity)
{
    static bool alarmOn = false;
    static bool anyUpdateReceived = false;

    if ((!alarmOn || !anyUpdateReceived) && humidity > ALARM_HUMIDITY_ON_VAL)
    {
        alarmOn = true;
        dataRouter_WriteBoolean(alarmHumidityKey, alarmOn, TIME_NOW);
    }
    else if (alarmOn && humidity < ALARM_HUMIDITY_OFF_VAL)
    {
        alarmOn = false;
        dataRouter_WriteBoolean(alarmHumidityKey, alarmOn, TIME_NOW);
    }
    anyUpdateReceived = true;
}

void checkLuminosityAlarm(double luminosity)
{
    static bool alarmOn = false;
    static bool anyUpdateReceived = false;

    if ((!alarmOn || !anyUpdateReceived) && luminosity > ALARM_LUMINOSITY_ON_VAL)
    {
        alarmOn = true;
        dataRouter_WriteBoolean(alarmLuminosityKey, alarmOn, TIME_NOW);
    }
    else if (alarmOn && luminosity < ALARM_LUMINOSITY_OFF_VAL)
    {
        alarmOn = false;
        dataRouter_WriteBoolean(alarmLuminosityKey, alarmOn, TIME_NOW);
    }
    anyUpdateReceived = true;
}

void checkOrientationAlarm(int32_t orientation)
{
    static bool alarmOn = false;
    static bool anyUpdateReceived = false;

    const int32_t upright = 5;
    bool newAlarmOn = orientation != upright;
    if (!anyUpdateReceived || newAlarmOn != alarmOn)
    {
        alarmOn = newAlarmOn;
        anyUpdateReceived = true;
        dataRouter_WriteBoolean(alarmOrientationKey, newAlarmOn, TIME_NOW);
    }
}
