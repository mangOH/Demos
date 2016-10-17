/**
 * @file
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"


#define DATA_ALARM_SHOCK       "container.alarmShock"
#define DATA_ALARM_TEMPERATURE "container.alarmTemperature"
#define DATA_ALARM_ORIENTATION "container.alarmOrientation"


/*
static void OnAlarmRaised
(
    dataRouter_DataType_t type,
    const char* key,
    void* context
)
{
    char messageBuffer[141] = "";

    if (strcmp(key, DATA_ALARM_SHOCK) == 0)
    {
        LE_FATAL_IF(type != DATAROUTER_FLOAT, "Unexpected data type received in alarm message.");

        double value;
        uint32_t timestamp;

        dataRouter_ReadFloat(DATA_ALARM_SHOCK, &value, &timestamp);
        sprintf(messageBuffer, "Shock alarm sounded, change of acceleration %f.", value);
    }
    else if (strcmp(key, DATA_ALARM_TEMPERATURE) == 0)
    {
        LE_FATAL_IF(type != DATAROUTER_BOOLEAN, "Unexpected data type received in alarm message.");

        bool value;
        uint32_t timestamp;

        dataRouter_ReadBoolean(DATA_ALARM_TEMPERATURE, &value, &timestamp);

        if (value)
        {
            sprintf(messageBuffer, "The container temperature is out of range!");
        }
        else
        {
            sprintf(messageBuffer, "The container temperature is back within range.");
        }
    }
    else if (strcmp(key, DATA_ALARM_ORIENTATION) == 0)
    {
        LE_FATAL_IF(type != DATAROUTER_BOOLEAN, "Unexpected data type received in alarm message.");

        bool value;
        uint32_t timestamp;

        dataRouter_ReadBoolean(DATAROUTER_BOOLEAN, &value, &timestamp);

        if (value)
        {
            sprintf(messageBuffer, "The container is in an unexpected orientation!");
        }
        else
        {
            sprintf(messageBuffer, "The container is now oriented correctly.");
        }
    }
    else
    {
        LE_FATAL("Unexpected alarm raised: %s", key);
    }

    twitter_Tweet(messageBuffer);
}
*/



COMPONENT_INIT
{
    //dataRouter_SessionStart("", "", false, DATAROUTER_CACHE);

    //dataRouter_AddDataUpdateHandler(DATA_ALARM_SHOCK, OnAlarmRaised, NULL);
    //dataRouter_AddDataUpdateHandler(DATA_ALARM_TEMPERATURE, OnAlarmRaised, NULL);
    //dataRouter_AddDataUpdateHandler(DATA_ALARM_ORIENTATION, OnAlarmRaised, NULL);
}
