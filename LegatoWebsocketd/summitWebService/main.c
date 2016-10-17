/**
 * @file
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"
#include <stdio.h>

#include "le_timer.h"


const char temperatureKey[] = "temperature";
const char alarmTemperatureKey[] = "alarmTemperature";


/*
void onTimer(le_timer_Ref_t timer)
{
    printf("{ \"foo\": 17, \"bar\":[1, 2, 3] }\n");
}
*/

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

    printf("{ \"temperature\": %f }\n", temperatureVal);
}

static void AlarmTemperatureUpdateHandler(
    dataRouter_DataType_t type,
    const char* key,
    void* contextPtr
)
{
    LE_FATAL_IF(
        strcmp(key, alarmTemperatureKey) != 0,
        "temperature alarm update handler received update for unexpected data key (%s)",
        key);
    LE_FATAL_IF(
        type != DATAROUTER_FLOAT,
        "temperature alarm update handler received update with unexpected type (%d)",
        type);

    double temperatureVal;
    uint32_t timestamp;
    dataRouter_ReadFloat(alarmTemperatureKey, &temperatureVal, &timestamp);

    printf("{ \"alarmTemperature\": %f }\n", temperatureVal);
}

COMPONENT_INIT
{
    /*
    le_timer_Ref_t tr = le_timer_Create("summitWebService");
    le_timer_SetMsInterval(tr, 3000);
    le_timer_SetRepeat(tr, 0);
    le_timer_SetHandler(tr, &onTimer);
    le_timer_Start(tr);
    */

    dataRouter_DataUpdateHandlerRef_t temperatureHandlerRef =
        dataRouter_AddDataUpdateHandler(temperatureKey, TemperatureUpdateHandler, NULL);
    dataRouter_DataUpdateHandlerRef_t alarmTemperatureHandlerRef =
        dataRouter_AddDataUpdateHandler(alarmTemperatureKey, AlarmTemperatureUpdateHandler, NULL);

    (void)temperatureHandlerRef;
    (void)alarmTemperatureHandlerRef;
}

