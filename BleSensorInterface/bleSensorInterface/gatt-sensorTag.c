/**
 * @file
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. Use of this work is subject to license.
 */

// --------------------------------- INCLUDES
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "legato.h"
#include "interfaces.h"
#include "le_cfg_interface.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <math.h>
#include <sys/signalfd.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "alarms.h"
#include "sensorConversions.h"

// --------------------------------- DEFINES

// DataRouter/AirVantage keys
#define KEY_IR_OBJECT_TEMPERATURE       "a.bt.read.ir.objectTemperature"
#define KEY_IR_AMBIENT_TEMPERATURE      "a.bt.read.ir.ambientTemperature"
#define KEY_HUMIDITY_TEMPERATURE        "a.bt.read.humidity.temperature"
#define KEY_HUMIDITY_HUMIDITY           "a.bt.read.humidity.humidity"
#define KEY_BAROMETER_TEMPERATURE       "a.bt.read.barometer.temperature"
#define KEY_BAROMETER_PRESSURE          "a.bt.read.barometer.pressure"
#define KEY_OPTICAL_LUMINOSITY          "a.bt.read.luminosity"
#define KEY_MOVEMENT_GYROSCOPE_X        "a.bt.read.movement.gyroscope.x"
#define KEY_MOVEMENT_GYROSCOPE_Y        "a.bt.read.movement.gyroscope.y"
#define KEY_MOVEMENT_GYROSCOPE_Z        "a.bt.read.movement.gyroscope.z"
#define KEY_MOVEMENT_ACCELEROMETER_X    "a.bt.read.movement.accelerometer.x"
#define KEY_MOVEMENT_ACCELEROMETER_Y    "a.bt.read.movement.accelerometer.y"
#define KEY_MOVEMENT_ACCELEROMETER_Z    "a.bt.read.movement.accelerometer.z"
#define KEY_MOVEMENT_MAGNETOMETER_X     "a.bt.read.movement.magnetometer.x"
#define KEY_MOVEMENT_MAGNETOMETER_Y     "a.bt.read.movement.magnetometer.y"
#define KEY_MOVEMENT_MAGNETOMETER_Z     "a.bt.read.movement.magnetometer.z"
#define KEY_SHOCK                       "a.bt.calc.shock"
#define KEY_ORIENTATION                 "a.bt.calc.orientation"
#define KEY_COMPASS                     "a.bt.calc.compass"
#define KEY_BUZZER                      "buzzer.enable"
#define KEY_RED_LED                     "redLED.enable"
#define KEY_GREEN_LED                   "greenLED.enable"

#define PUBLISH_PERIOD_IN_MS 10000

#define PIPE_READ_END_INDEX 0
#define PIPE_WRITE_END_INDEX 1

// Movement sensor notification is 18 bytes.  All others are smaller.
#define MAX_NOTIFICATION_SIZE 18


// Convenience macros for initializing struct HandleLookup values.
// NOTE: We don't need to permanently store the handle of a primary service, so set handleStorage
// to NULL.
#define LOOKUP_PRIMARY_SERVICE(_uuid_) { .isPrimaryService=true, .uuid=_uuid_, .handleStorage=NULL }
#define LOOKUP_CHARACTERISTIC(_uuid_, _storagePtr_) { .isPrimaryService=false, .uuid=_uuid_, .handleStorage=_storagePtr_ }


// --------------------------------- DATA TYPES

// A type representing a handle to lookup
struct HandleLookup
{
    bool isPrimaryService;
    const char* uuid;
    uint16_t* handleStorage;
};

typedef struct
{
    struct
    {
        double ambientTemperatureInCelcius;
        double objectTemperatureInCelcius;
    } irTemperature;
    struct
    {
        double temperatureInCelcius;
        double humidityInRelativePercent;
    } humidity;
    struct
    {
        double temperatureInCelcius;
        double pressureInHectoPascal;
    } barometricPressure;
    struct
    {
        double luminosityInLux;
    } optical;
    struct
    {
        struct
        {
            double x;
            double y;
            double z;
        } gyroInDegreesPerSecond;
        struct
        {
            double x;
            double y;
            double z;
        } accelerometerInG;
        struct
        {
            double x;
            double y;
            double z;
        } magnetometerInMicroTesla;
    } movement;
} SensorReadings;

typedef struct
{
    double compassAngle;
    double shock;
    uint8_t orientation;
} SensorCalculation;


enum ProgramState
{
    STATE_BEGIN,
    STATE_WAIT_FOR_ATTEMPTING,
    STATE_WAIT_FOR_CONNECT_SUCCESS,
    STATE_HANDLE_LOOKUP_BEGIN,
    STATE_HANDLE_LOOKUP_WAIT_FOR_RANGE,
    STATE_HANDLE_LOOKUP_WAIT_FOR_HANDLES,
    STATE_WAIT_FOR_NOTIFICATIONS
};


// --------------------------------- STATIC FUNCTION DECLARATIONS
static void waitForConnectSuccessTimeoutHandler(le_timer_Ref_t timer);
static void waitForRangeTimeoutHandler(le_timer_Ref_t timer);
static void waitForHandleMappingsTimeoutHandler(le_timer_Ref_t timer);
static void publishTimerHandler(le_timer_Ref_t timer);
static void stdoutLineHandler(const char* line, size_t lineLength);
static void continueInitialization(void);
static bool tryParseNotification(
    const char* line,
    size_t lineLength,
    uint16_t* notificationHandle,
    uint8_t* notificationData,
    size_t* notificationSize);
static bool tryParseHandleRange(
    const char* line, size_t lineLength, uint16_t* startHandle, uint16_t* endHandle);
static bool tryParseHandleMapping(
    const char* line, size_t lineLength, uint16_t* handle, char* uuid);
static void notificationLineHandler(const char* line, size_t lineLength);
static int initPipes(void);
static void charWriteCmd(uint16_t handle, const uint8_t* data, size_t dataLength);
static void sendCommand(const char* cmd);
static void configureIrTemperatureSensor(void);
static void configureHumiditySensor(void);
static void configureBarometricPressureSensor(void);
static void configureIO(void);
static void configureOpticalSensor(void);
static void configureMovementSensor(void);
static void stdinHandler(int fd, short events);
static void stdoutHandler(int fd, short events);
static void stderrHandler(int fd, short events);
static void childProcess(void);
static void parentProcess(void);
static void logData(const SensorReadings* reading, const SensorCalculation* calculation);
static bool approxEq(float v1, float v2, float threshold);
static void performSensorCalculation(
    SensorCalculation* calculation,
    const SensorReadings* currentReading,
    const SensorReadings* previousReading);
static void updateSensorTagOutputs(void);
static void outputUpdateHandler(dataRouter_DataType_t type, const char* key, void* context);
static void sigChldHandler(int signal);
static void dumpBytes(const uint8_t* bytes, size_t numBytes) __attribute__((unused));

// --------------------------------- STATIC VARIABLES

// SensorTag Primary service UUIDs
const char irTemperatureSensorServiceUUID[]      = "f000aa00-0451-4000-b000-000000000000";
const char humiditySensorServiceUUID[]           = "f000aa20-0451-4000-b000-000000000000";
const char barometricPressureSensorServiceUUID[] = "f000aa40-0451-4000-b000-000000000000";
const char ioServiceUUID[]                       = "f000aa64-0451-4000-b000-000000000000";
const char opticalSensorServiceUUID[]            = "f000aa70-0451-4000-b000-000000000000";
const char movementSensorServiceUUID[]           = "f000aa80-0451-4000-b000-000000000000";
const char registerServiceUUID[]                 = "f000ac00-0451-4000-b000-000000000000";
const char connectionControlServiceUUID[]        = "f000ccc0-0451-4000-b000-000000000000";
const char oadServiceUUID[]                      = "f000ffc0-0451-4000-b000-000000000000";
const char genericAccessServiceUUID[]            = "00001800-0000-1000-8000-00805f9b34fb";
const char genericAttributeServiceUUID[]         = "00001801-0000-1000-8000-00805f9b34fb";
const char deviceInformationServiceUUID[]        = "0000180a-0000-1000-8000-00805f9b34fb";
const char batteryServiceUUID[]                  = "0000180f-0000-1000-8000-00805f9b34fb";
const char simpleKeysServiceUUID[]               = "0000ffe0-0000-1000-8000-00805f9b34fb";

// Characteristics
const char clientCharacteristicConfigurationDescriptorUUID[] =
    "00002902-0000-1000-8000-00805f9b34fb";

const char irTemperatureSensorDataCharacteristicUUID[]    = "f000aa01-0451-4000-b000-000000000000";
const char irTemperatureSensorConfigurationCharacteristicUUID[] =
    "f000aa02-0451-4000-b000-000000000000";
const char irTemperatureSensorPeriodCharacteristicUUID[]  = "f000aa03-0451-4000-b000-000000000000";

const char humiditySensorDataCharacteristicUUID[]         = "f000aa21-0451-4000-b000-000000000000";
const char humiditySensorConfigurationCharacteristicUUID[] =
    "f000aa22-0451-4000-b000-000000000000";
const char humiditySensorPeriodCharacteristicUUID[]       = "f000aa23-0451-4000-b000-000000000000";

const char barometricPressureSensorDataCharacteristicUUID[] =
    "f000aa41-0451-4000-b000-000000000000";
const char barometricPressureSensorConfigurationCharacteristicUUID[] =
    "f000aa42-0451-4000-b000-000000000000";
const char barometricPressureSensorPeriodCharacteristicUUID[] =
    "f000aa44-0451-4000-b000-000000000000";

const char ioDataCharacteristicUUID[]                     = "f000aa65-0451-4000-b000-000000000000";
const char ioConfigurationCharacteristicUUID[]            = "f000aa66-0451-4000-b000-000000000000";

const char opticalSensorDataCharacteristicUUID[]          = "f000aa71-0451-4000-b000-000000000000";
const char opticalSensorConfigurationCharacteristicUUID[] = "f000aa72-0451-4000-b000-000000000000";
const char opticalSensorPeriodCharacteristicUUID[]        = "f000aa73-0451-4000-b000-000000000000";

const char movementSensorDataCharacteristicUUID[]         = "f000aa81-0451-4000-b000-000000000000";
const char movementSensorConfigurationCharacteristicUUID[] =
    "f000aa82-0451-4000-b000-000000000000";
const char movementSensorPeriodCharacteristicUUID[]       = "f000aa83-0451-4000-b000-000000000000";

const char simpleKeysDataCharacteristicUUID[] = "0000ffe0-0000-1000-8000-00805f9b34fb";

static struct
{
    enum ProgramState programState;
    struct
    {
        size_t currentPrimaryServiceOffset;
        le_timer_Ref_t commandResponseTimer;
    } initialization;
    char deviceMacAddr[(6 * 2) + ((6 - 1) * 1) + 1]; // digits + colons + terminator
    struct
    {
        int childStdout[2];
        int childStderr[2];
        int childStdin[2];
    } pipes;
    struct
    {
        le_fdMonitor_Ref_t childStdin;
        le_fdMonitor_Ref_t childStdout;
        le_fdMonitor_Ref_t childStderr;
    } fdMonitors;
    struct
    {
        bool redLedOn;
        bool greenLedOn;
        bool buzzerOn;
    } outputStates;
    le_timer_Ref_t publishTimer;
    SensorReadings sensorReadings;
    SensorReadings previousSensorReadings;
    SensorCalculation sensorCalculations;
    // All of the handles that we care about.  After initialization is complete, all of the values
    // will be populated.
    struct
    {
        struct
        {
            uint16_t data;
            uint16_t notification;
            uint16_t configuration;
            uint16_t period;
        } irTemperatureSensor;
        struct
        {
            uint16_t data;
            uint16_t notification;
            uint16_t configuration;
            uint16_t period;
        } humiditySensor;
        struct
        {
            uint16_t data;
            uint16_t notification;
            uint16_t configuration;
            uint16_t period;
        } barometricPressureSensor;
        struct
        {
            uint16_t data;
            uint16_t configuration;
        } io;
        struct
        {
            uint16_t data;
            uint16_t notification;
            uint16_t configuration;
            uint16_t period;
        } opticalSensor;
        struct
        {
            uint16_t data;
            uint16_t notification;
            uint16_t configuration;
            uint16_t period;
        } movementSensor;
    } handles;
} m;

// Table of handles to lookup.  By convention, all characteristics are assumed to be members of the
// primary service that preceded it.
static const struct HandleLookup handlesToLookup[] =
{
    LOOKUP_PRIMARY_SERVICE(irTemperatureSensorServiceUUID),
    LOOKUP_CHARACTERISTIC(irTemperatureSensorDataCharacteristicUUID, &m.handles.irTemperatureSensor.data),
    LOOKUP_CHARACTERISTIC(clientCharacteristicConfigurationDescriptorUUID, &m.handles.irTemperatureSensor.notification),
    LOOKUP_CHARACTERISTIC(irTemperatureSensorConfigurationCharacteristicUUID, &m.handles.irTemperatureSensor.configuration),
    LOOKUP_CHARACTERISTIC(irTemperatureSensorPeriodCharacteristicUUID, &m.handles.irTemperatureSensor.period),

    LOOKUP_PRIMARY_SERVICE(humiditySensorServiceUUID),
    LOOKUP_CHARACTERISTIC(humiditySensorDataCharacteristicUUID, &m.handles.humiditySensor.data),
    LOOKUP_CHARACTERISTIC(clientCharacteristicConfigurationDescriptorUUID, &m.handles.humiditySensor.notification),
    LOOKUP_CHARACTERISTIC(humiditySensorConfigurationCharacteristicUUID, &m.handles.humiditySensor.configuration),
    LOOKUP_CHARACTERISTIC(humiditySensorPeriodCharacteristicUUID, &m.handles.humiditySensor.period),

    LOOKUP_PRIMARY_SERVICE(barometricPressureSensorServiceUUID),
    LOOKUP_CHARACTERISTIC(barometricPressureSensorDataCharacteristicUUID, &m.handles.barometricPressureSensor.data),
    LOOKUP_CHARACTERISTIC(clientCharacteristicConfigurationDescriptorUUID, &m.handles.barometricPressureSensor.notification),
    LOOKUP_CHARACTERISTIC(barometricPressureSensorConfigurationCharacteristicUUID, &m.handles.barometricPressureSensor.configuration),
    LOOKUP_CHARACTERISTIC(barometricPressureSensorPeriodCharacteristicUUID, &m.handles.barometricPressureSensor.period),

    LOOKUP_PRIMARY_SERVICE(ioServiceUUID),
    LOOKUP_CHARACTERISTIC(ioDataCharacteristicUUID, &m.handles.io.data),
    LOOKUP_CHARACTERISTIC(ioConfigurationCharacteristicUUID, &m.handles.io.configuration),

    LOOKUP_PRIMARY_SERVICE(opticalSensorServiceUUID),
    LOOKUP_CHARACTERISTIC(opticalSensorDataCharacteristicUUID, &m.handles.opticalSensor.data),
    LOOKUP_CHARACTERISTIC(clientCharacteristicConfigurationDescriptorUUID, &m.handles.opticalSensor.notification),
    LOOKUP_CHARACTERISTIC(opticalSensorConfigurationCharacteristicUUID, &m.handles.opticalSensor.configuration),
    LOOKUP_CHARACTERISTIC(opticalSensorPeriodCharacteristicUUID, &m.handles.opticalSensor.period),

    LOOKUP_PRIMARY_SERVICE(movementSensorServiceUUID),
    LOOKUP_CHARACTERISTIC(movementSensorDataCharacteristicUUID, &m.handles.movementSensor.data),
    LOOKUP_CHARACTERISTIC(clientCharacteristicConfigurationDescriptorUUID, &m.handles.movementSensor.notification),
    LOOKUP_CHARACTERISTIC(movementSensorConfigurationCharacteristicUUID, &m.handles.movementSensor.configuration),
    LOOKUP_CHARACTERISTIC(movementSensorPeriodCharacteristicUUID, &m.handles.movementSensor.period)
};


static void waitForConnectSuccessTimeoutHandler(le_timer_Ref_t timer)
{
    if (m.programState == STATE_WAIT_FOR_ATTEMPTING ||
        m.programState == STATE_WAIT_FOR_CONNECT_SUCCESS)
    {
        LE_ERROR("Failed to connect to Bluetooth device.  Retrying.");
        m.programState = STATE_BEGIN;
        continueInitialization();
    }
    else
    {
        LE_WARN(
            "Received wait for connect success timeout in unexpected state (%d)", m.programState);
    }
}

static void waitForRangeTimeoutHandler(le_timer_Ref_t timer)
{
    if (m.programState == STATE_HANDLE_LOOKUP_WAIT_FOR_RANGE)
    {
        LE_FATAL("Timed out waiting for handle range");
    }
    else
    {
        LE_WARN(
            "Received wait for handle range timeout in unexpected state (%d)", m.programState);
    }
}

static void waitForHandleMappingsTimeoutHandler(le_timer_Ref_t timer)
{
    if (m.programState == STATE_HANDLE_LOOKUP_WAIT_FOR_HANDLES)
    {
        size_t i;
        for (
            i = m.initialization.currentPrimaryServiceOffset + 1;
            i < NUM_ARRAY_MEMBERS(handlesToLookup) && !handlesToLookup[i].isPrimaryService;
            i++)
        {
            LE_FATAL_IF(
                *(handlesToLookup[i].handleStorage) == 0,
                "Failed to lookup handle for characteristic uuid=%s within primary service=%s",
                handlesToLookup[i].uuid,
                handlesToLookup[m.initialization.currentPrimaryServiceOffset].uuid);
        }
        m.initialization.currentPrimaryServiceOffset = i;
        m.programState = STATE_HANDLE_LOOKUP_BEGIN;
        continueInitialization();
    }
}

static void publishTimerHandler(le_timer_Ref_t timer)
{
    performSensorCalculation(
        &m.sensorCalculations,
        &m.sensorReadings,
        &m.previousSensorReadings);
    memcpy(&m.previousSensorReadings, &m.sensorReadings, sizeof(SensorReadings));

    logData(&m.sensorReadings, &m.sensorCalculations);

    // Publish data
    time_t now = time(NULL);

    // Readings

    dataRouter_WriteFloat(
        KEY_IR_OBJECT_TEMPERATURE, m.sensorReadings.irTemperature.objectTemperatureInCelcius, now);
    dataRouter_WriteFloat(
        KEY_IR_AMBIENT_TEMPERATURE,
        m.sensorReadings.irTemperature.ambientTemperatureInCelcius,
        now);
    dataRouter_WriteFloat(
        KEY_HUMIDITY_TEMPERATURE, m.sensorReadings.humidity.temperatureInCelcius, now);
    dataRouter_WriteFloat(
        KEY_HUMIDITY_HUMIDITY, m.sensorReadings.humidity.humidityInRelativePercent, now);
    dataRouter_WriteFloat(
        KEY_BAROMETER_TEMPERATURE, m.sensorReadings.barometricPressure.temperatureInCelcius, now);
    dataRouter_WriteFloat(
        KEY_BAROMETER_PRESSURE, m.sensorReadings.barometricPressure.pressureInHectoPascal, now);
    dataRouter_WriteFloat(
        KEY_OPTICAL_LUMINOSITY, m.sensorReadings.optical.luminosityInLux, now);
    dataRouter_WriteFloat(
        KEY_MOVEMENT_GYROSCOPE_X, m.sensorReadings.movement.gyroInDegreesPerSecond.x, now);
    dataRouter_WriteFloat(
        KEY_MOVEMENT_GYROSCOPE_Y, m.sensorReadings.movement.gyroInDegreesPerSecond.y, now);
    dataRouter_WriteFloat(
        KEY_MOVEMENT_GYROSCOPE_Z, m.sensorReadings.movement.gyroInDegreesPerSecond.z, now);
    dataRouter_WriteFloat(
        KEY_MOVEMENT_ACCELEROMETER_X, m.sensorReadings.movement.accelerometerInG.x, now);
    dataRouter_WriteFloat(
        KEY_MOVEMENT_ACCELEROMETER_Y, m.sensorReadings.movement.accelerometerInG.y, now);
    dataRouter_WriteFloat(
        KEY_MOVEMENT_ACCELEROMETER_Z, m.sensorReadings.movement.accelerometerInG.z, now);
    dataRouter_WriteFloat(
        KEY_MOVEMENT_MAGNETOMETER_X, m.sensorReadings.movement.magnetometerInMicroTesla.x, now);
    dataRouter_WriteFloat(
        KEY_MOVEMENT_MAGNETOMETER_Y, m.sensorReadings.movement.magnetometerInMicroTesla.y, now);
    dataRouter_WriteFloat(
        KEY_MOVEMENT_MAGNETOMETER_Z, m.sensorReadings.movement.magnetometerInMicroTesla.z, now);

    // Calculations

    dataRouter_WriteFloat(KEY_SHOCK, m.sensorCalculations.shock, now);
    dataRouter_WriteInteger(KEY_ORIENTATION, m.sensorCalculations.orientation, now);
    dataRouter_WriteFloat(KEY_COMPASS, m.sensorCalculations.compassAngle, now);

    // check alarms
    checkTemperatureAlarm(m.sensorReadings.humidity.temperatureInCelcius);
    checkHumidityAlarm(m.sensorReadings.humidity.humidityInRelativePercent);
    checkLuminosityAlarm(m.sensorReadings.optical.luminosityInLux);
    checkShockAlarm(m.sensorCalculations.shock);
    checkOrientationAlarm(m.sensorCalculations.orientation);
}

static void stdoutLineHandler(const char* line, size_t lineLength)
{
    char commandPrefix[64];
    int snprintfResult = snprintf(
        commandPrefix,
        sizeof(commandPrefix),
        "[%s][LE]> ",
        m.deviceMacAddr);
    LE_ASSERT(snprintfResult > 0);
    if (strncmp(line, commandPrefix, snprintfResult) == 0)
    {
        LE_INFO("Ignoring command echo: %s", line);
        return;
    }

    // This is the version of the prompt with blue highlighting indicating connectivity
    snprintfResult = snprintf(
        commandPrefix,
        sizeof(commandPrefix),
        "\e[0;94m[%s]\e[0m[LE]> ",
        m.deviceMacAddr);
    LE_ASSERT(snprintfResult > 0);
    if (strncmp(line, commandPrefix, snprintfResult) == 0)
    {
        LE_INFO("Ignoring command echo: %s", line);
        return;
    }

    const char strangeCommandPrefix[] = "<[0m[LE]> ";
    if (strncmp(line, strangeCommandPrefix, strlen(strangeCommandPrefix)) == 0)
    {
        LE_INFO("Ignoring command echo: %s", line);
        return;
    }

    LE_INFO("Processing because this isn't a command echo: '%s'", line);

    switch (m.programState)
    {
        case STATE_WAIT_FOR_ATTEMPTING:
        {
            const char attempting[] = "Attempting to connect to ";
            if (strncmp(line, attempting, sizeof(attempting) - 1) == 0)
            {
                m.programState = STATE_WAIT_FOR_CONNECT_SUCCESS;
            }
            else if (strcmp(line, "Connection successful") == 0)
            {
                // Sometimes the "Attempting to connect to <MAC>" message never appears, so handle
                // the case of "Connection successful" in this state as well.
                le_timer_Stop(m.initialization.commandResponseTimer);
                m.programState = STATE_HANDLE_LOOKUP_BEGIN;
                continueInitialization();
            }
            else
            {
                LE_FATAL("Received unexpected output while trying to connect (%s)", line);
            }
            break;
        }

        case STATE_WAIT_FOR_CONNECT_SUCCESS:
        {
            LE_FATAL_IF(
                strcmp(line, "Connection successful") != 0,
                "Received unexpected output while trying to connect (%s)",
                line);
            le_timer_Stop(m.initialization.commandResponseTimer);
            m.programState = STATE_HANDLE_LOOKUP_BEGIN;
            continueInitialization();
            break;
        }

        case STATE_HANDLE_LOOKUP_WAIT_FOR_RANGE:
        {
            uint16_t startHandle;
            uint16_t endHandle;
            LE_FATAL_IF(
                !tryParseHandleRange(line, lineLength, &startHandle, &endHandle),
                "Received unexpected output while waiting for a handle range (%s)",
                line);
            le_timer_Stop(m.initialization.commandResponseTimer);
            char charDescCmd[128];
            sprintf(charDescCmd, "char-desc 0x%04x 0x%04x", startHandle, endHandle);
            LE_ASSERT_OK(le_timer_SetMsInterval(m.initialization.commandResponseTimer, 2000));
            LE_ASSERT_OK(
                le_timer_SetHandler(
                    m.initialization.commandResponseTimer, waitForHandleMappingsTimeoutHandler));
            LE_ASSERT_OK(le_timer_Start(m.initialization.commandResponseTimer));
            sendCommand(charDescCmd);
            le_timer_Start(m.initialization.commandResponseTimer);
            m.programState = STATE_HANDLE_LOOKUP_WAIT_FOR_HANDLES;
            break;
        }

        case STATE_HANDLE_LOOKUP_WAIT_FOR_HANDLES:
        {
            uint16_t handle;
            char uuid[37];
            LE_FATAL_IF(
                !tryParseHandleMapping(line, lineLength, &handle, uuid),
                "Received unexpected output while waiting for a handle mapping (%s)",
                line);
            if (strlen(uuid) == 4)
            {
                // Convert abbreviated UUID, into full length one.
                char temp[5] = {0};
                strcpy(temp, uuid);
                snprintf(uuid, sizeof(uuid), "0000%s-0000-1000-8000-00805f9b34fb", temp);
            }
            for (
                size_t i = m.initialization.currentPrimaryServiceOffset + 1;
                i < NUM_ARRAY_MEMBERS(handlesToLookup) && !handlesToLookup[i].isPrimaryService;
                i++)
            {
                if (strcmp(uuid, handlesToLookup[i].uuid) == 0)
                {
                    *(handlesToLookup[i].handleStorage) = handle;
                    break;
                }
            }
            break;
        }

        case STATE_WAIT_FOR_NOTIFICATIONS:
            notificationLineHandler(line, lineLength);
            break;

        default:
            LE_FATAL("In unexpected state (%d)", m.programState);
            break;
    }
}

static void continueInitialization(void)
{
    switch (m.programState)
    {
        case STATE_BEGIN:
        {
            if (m.initialization.commandResponseTimer == NULL)
            {
                m.initialization.commandResponseTimer =
                    le_timer_Create("gatttool command response timer");
                LE_ASSERT_OK(le_timer_SetMsInterval(m.initialization.commandResponseTimer, 3000));
                LE_ASSERT_OK(
                    le_timer_SetHandler(
                        m.initialization.commandResponseTimer,
                        waitForConnectSuccessTimeoutHandler));
            }
            LE_ASSERT_OK(le_timer_Start(m.initialization.commandResponseTimer));
            sendCommand("connect");
            m.programState = STATE_WAIT_FOR_ATTEMPTING;
            break;
        }

        case STATE_HANDLE_LOOKUP_BEGIN:
        {
            if (m.initialization.currentPrimaryServiceOffset >= NUM_ARRAY_MEMBERS(handlesToLookup))
            {
                configureIrTemperatureSensor();
                configureHumiditySensor();
                configureBarometricPressureSensor();
                configureIO();
                configureOpticalSensor();
                configureMovementSensor();

                dataRouter_AddDataUpdateHandler(KEY_RED_LED, outputUpdateHandler, NULL);
                dataRouter_AddDataUpdateHandler(KEY_GREEN_LED, outputUpdateHandler, NULL);
                dataRouter_AddDataUpdateHandler(KEY_BUZZER, outputUpdateHandler, NULL);

                m.publishTimer = le_timer_Create("bleSensorInterface publish");
                LE_ASSERT(le_timer_SetHandler(m.publishTimer, &publishTimerHandler) == LE_OK);
                LE_ASSERT(le_timer_SetMsInterval(m.publishTimer, PUBLISH_PERIOD_IN_MS) == LE_OK);
                LE_ASSERT(le_timer_SetRepeat(m.publishTimer, 0) == LE_OK); // repeat forever
                LE_ASSERT(le_timer_Start(m.publishTimer) == LE_OK);

                m.programState = STATE_WAIT_FOR_NOTIFICATIONS;
            }
            else
            {
                const struct HandleLookup* currentPrimaryServiceLookup = &handlesToLookup[m.initialization.currentPrimaryServiceOffset];
                LE_FATAL_IF(!currentPrimaryServiceLookup->isPrimaryService, "Expected primary service");
                char primaryCmd[128] = "primary ";
                strcat(primaryCmd, currentPrimaryServiceLookup->uuid);
                LE_ASSERT_OK(le_timer_SetMsInterval(m.initialization.commandResponseTimer, 2000));
                LE_ASSERT_OK(
                    le_timer_SetHandler(
                        m.initialization.commandResponseTimer, waitForRangeTimeoutHandler));
                LE_ASSERT_OK(le_timer_Start(m.initialization.commandResponseTimer));
                sendCommand(primaryCmd);
                m.programState = STATE_HANDLE_LOOKUP_WAIT_FOR_RANGE;
            }
            break;
        }

        default:
            LE_FATAL("In unexpected state (%d)", m.programState);
            break;
    }
}


static bool tryParseNotification
(
    const char* line,
    size_t lineLength,
    uint16_t* notificationHandle,
    uint8_t* notificationData,
    size_t* notificationSize
)
{
    // Expect: Notification handle = 0x0024 value: 00 aa 11 bb
    char valueString[lineLength];
    size_t bytesParsed = 0;
    if(sscanf(
            line,
            "Notification handle = 0x%04" SCNx16 " value: %[0-9 a-z]",
            notificationHandle,
            valueString) != 2)
    {
        return false;
    }
    const size_t valueLength = strlen(valueString);
    *notificationSize = 0;
    for (size_t i = 0; i < valueLength; i += 3)
    {
        if (*notificationSize >= MAX_NOTIFICATION_SIZE)
        {
            LE_WARN("Cannot fit all of the notification data in the buffer");
            return false;
        }
        if (sscanf(&valueString[i], "%02" SCNx8, &(notificationData[bytesParsed])) != 1)
        {
            LE_WARN("Failed parsing on what appeared to be notification data");
            return false;
        }
        bytesParsed++;
    }

    *notificationSize = bytesParsed;
    return true;
}

static bool tryParseHandleRange(
    const char* line, size_t lineLength, uint16_t* startHandle, uint16_t* endHandle)
{
    const int numScanned = sscanf(
        line,
        "Starting handle: 0x%04" SCNx16 " Ending handle: 0x%04" SCNx16,
        startHandle,
        endHandle);
    return numScanned == 2;
}

// TODO: make sure we don't overrun uuid
static bool tryParseHandleMapping(
    const char* line, size_t lineLength, uint16_t* handle, char* uuid)
{
    const int numScanned = sscanf(line, "handle: 0x%04" SCNx16 ", uuid: %s", handle, uuid);
    return numScanned == 2;
}

static void notificationLineHandler(const char* line, size_t lineLength)
{
    uint16_t notificationHandle;
    uint8_t notificationData[MAX_NOTIFICATION_SIZE];
    size_t notificationSize;
    LE_FATAL_IF(
        !tryParseNotification(line, lineLength, &notificationHandle, notificationData, &notificationSize),
        "Received a line of output which is not a notification (%s)",
        line);
    if (notificationHandle == m.handles.irTemperatureSensor.data)
    {
        LE_FATAL_IF(
            notificationSize != 4,
            "IR temperature sensor notification provided %d bytes, but 2 bytes were "
            "expected",
            notificationSize);
        convertIRTemperatureSensorData(
            notificationData,
            &m.sensorReadings.irTemperature.ambientTemperatureInCelcius,
            &m.sensorReadings.irTemperature.objectTemperatureInCelcius);
    }
    else if (notificationHandle == m.handles.humiditySensor.data)
    {
        LE_FATAL_IF(
            notificationSize != 4,
            "Humidity sensor notification provided %d bytes, but 2 bytes were expected",
            notificationSize);
        convertHumiditySensorData(
            notificationData,
            &m.sensorReadings.humidity.temperatureInCelcius,
            &m.sensorReadings.humidity.humidityInRelativePercent);
    }
    else if (notificationHandle == m.handles.barometricPressureSensor.data)
    {
        LE_FATAL_IF(
            notificationSize != 6,
            "Barometric pressure sensor notification provided %d bytes, but 3 bytes were "
            "expected",
            notificationSize);
        convertBarometricPressureSensorData(
            notificationData,
            &m.sensorReadings.barometricPressure.temperatureInCelcius,
            &m.sensorReadings.barometricPressure.pressureInHectoPascal);
    }
    else if (notificationHandle == m.handles.opticalSensor.data)
    {
        LE_FATAL_IF(
            notificationSize != 2,
            "Optical sensor notification provided %d bytes, but 2 bytes were expected",
            notificationSize);
        convertOpticalSensorData(notificationData, &m.sensorReadings.optical.luminosityInLux);
    }
    else if (notificationHandle == m.handles.movementSensor.data)
    {
        LE_FATAL_IF(
            notificationSize != 18,
            "Movement sensor notification provided %d bytes, but 18 bytes were expected",
            notificationSize);
        convertMovementSensorData(
            notificationData,
            ACC_RANGE_8G,
            &m.sensorReadings.movement.accelerometerInG.x,
            &m.sensorReadings.movement.accelerometerInG.y,
            &m.sensorReadings.movement.accelerometerInG.z,
            &m.sensorReadings.movement.gyroInDegreesPerSecond.x,
            &m.sensorReadings.movement.gyroInDegreesPerSecond.y,
            &m.sensorReadings.movement.gyroInDegreesPerSecond.z,
            &m.sensorReadings.movement.magnetometerInMicroTesla.x,
            &m.sensorReadings.movement.magnetometerInMicroTesla.y,
            &m.sensorReadings.movement.magnetometerInMicroTesla.z);
    }
    else
    {
        LE_FATAL("Received unexpected notification for handle (%d)", notificationHandle);
    }
}

static int initPipes(void)
{
    int r;
    r = pipe(m.pipes.childStdout);
    if (r != 0)
    {
        return r;
    }
    r = pipe(m.pipes.childStderr);
    if (r != 0)
    {
        return r;
    }
    r = pipe(m.pipes.childStdin);
    return r;
}


static void charWriteCmd(uint16_t handle, const uint8_t* data, size_t dataLength)
{
    char buffer[32];
    sprintf(buffer, "char-write-cmd 0x%04" PRIx16 " ", handle);
    size_t bufferLen = strlen(buffer);
    ssize_t writeResult = write(m.pipes.childStdin[PIPE_WRITE_END_INDEX], buffer, bufferLen);
    LE_ASSERT(writeResult == bufferLen);
    for (size_t i = 0; i < dataLength; i++)
    {
        sprintf(buffer, "%02" PRIx8, data[i]);
        LE_ASSERT(write(m.pipes.childStdin[PIPE_WRITE_END_INDEX], buffer, 2) == 2);
    }
    LE_ASSERT(write(m.pipes.childStdin[PIPE_WRITE_END_INDEX], "\n", 1) == 1);
}

static void sendCommand(const char* cmd)
{
    size_t cmdLength = strlen(cmd);
    char* cmdBuffer = calloc(cmdLength + 2, 1);
    LE_ASSERT(cmdBuffer != NULL);
    memcpy(cmdBuffer, cmd, cmdLength);
    cmdBuffer[cmdLength] = '\n';

    ssize_t writeResult = write(m.pipes.childStdin[PIPE_WRITE_END_INDEX], cmdBuffer, cmdLength + 1);
    LE_FATAL_IF(
        writeResult != cmdLength + 1,
        "write result was %d when trying to write %d bytes",
        writeResult,
        cmdLength);
    free(cmdBuffer);
}

static void configureIrTemperatureSensor(void)
{
    LE_DEBUG("Configuring IR temperature sensor");
    const uint8_t period[] = {200}; // n * 10ms
    charWriteCmd(m.handles.irTemperatureSensor.period, period, sizeof(period));
    const uint8_t enableNotifications[] = {0x01, 0x00};
    charWriteCmd(
        m.handles.irTemperatureSensor.notification,
        enableNotifications,
        sizeof(enableNotifications));
    const uint8_t configuration[] = {0x01};
    charWriteCmd(
        m.handles.irTemperatureSensor.configuration, configuration, sizeof(configuration));
}

static void configureHumiditySensor(void)
{
    LE_DEBUG("Configuring humidity sensor");
    const uint8_t period[] = {200}; // n * 10ms
    charWriteCmd(m.handles.humiditySensor.period, period, sizeof(period));
    const uint8_t enableNotifications[] = {0x01, 0x00};
    charWriteCmd(
        m.handles.humiditySensor.notification, enableNotifications, sizeof(enableNotifications));
    const uint8_t configuration[] = {0x01};
    charWriteCmd(m.handles.humiditySensor.configuration, configuration, sizeof(configuration));
}

static void configureBarometricPressureSensor(void)
{
    LE_DEBUG("Configuring barometric pressure sensor");
    const uint8_t period[] = {200}; // n * 10ms
    charWriteCmd(m.handles.barometricPressureSensor.period, period, sizeof(period));
    const uint8_t enableNotifications[] = {0x01, 0x00};
    charWriteCmd(
        m.handles.barometricPressureSensor.notification,
        enableNotifications,
        sizeof(enableNotifications));
    const uint8_t configuration[] = {0x01};
    charWriteCmd(
        m.handles.barometricPressureSensor.configuration, configuration, sizeof(configuration));
}

static void configureIO(void)
{
    LE_DEBUG("Configuring I/O");
    const uint8_t data[] = {0x00}; // Turn buzzer and LEDs off
    charWriteCmd(m.handles.io.data, data, sizeof(data));
    const uint8_t configuration[] = {0x01}; // set to remote mode
    charWriteCmd(m.handles.io.configuration, configuration, sizeof(configuration));
}

static void configureOpticalSensor(void)
{
    LE_DEBUG("Configuring optical sensor");
    const uint8_t period[] = {200}; // n * 10ms
    charWriteCmd(m.handles.opticalSensor.period, period, sizeof(period));
    const uint8_t enableNotifications[] = {0x01, 0x00};
    charWriteCmd(
        m.handles.opticalSensor.notification, enableNotifications, sizeof(enableNotifications));
    const uint8_t configuration[] = {0x01};
    charWriteCmd(m.handles.opticalSensor.configuration, configuration, sizeof(configuration));
}

static void configureMovementSensor(void)
{
    LE_INFO("Configuring movement sensor - data handle is %d", m.handles.movementSensor.data);
    //const uint8_t period[] = {200}; // n * 10ms
    // For some reason the motion sensor doesn't work when the period is set to 200
    const uint8_t period[] = {0x64}; // n * 10ms
    charWriteCmd(m.handles.movementSensor.period, period, sizeof(period));
    const uint8_t enableNotifications[] = {0x01, 0x00};
    charWriteCmd(
        m.handles.movementSensor.notification, enableNotifications, sizeof(enableNotifications));
    // Ensable wake on motion and +-8G range.  It seems that it doesn't matter what range setting
    // is written into the SensorTag and +-8G is always used.
    const uint8_t configuration[] = {0xFF, 0x02};
    charWriteCmd(m.handles.movementSensor.configuration, configuration, sizeof(configuration));
}

static void stdinHandler(int fd, short events)
{
    LE_ASSERT(fd == m.pipes.childStdin[PIPE_WRITE_END_INDEX]);
    if (events & POLLERR)
    {
        LE_FATAL("Read end of childStdinPipe is closed");
    }
}

static void dumpBytes(const uint8_t* bytes, size_t numBytes)
{
    while (numBytes != 0)
    {
        switch (numBytes)
        {
            case 1:
                LE_INFO("  %02x | %c", bytes[0], bytes[0]);
                numBytes -= 1;
                bytes += 1;
                break;

            case 2:
                LE_INFO("  %02x %02x | %c %c", bytes[0], bytes[1], bytes[0], bytes[1]);
                numBytes -= 2;
                bytes += 2;
                break;

            case 3:
                LE_INFO(
                    "  %02x %02x %02x | %c %c %c",
                    bytes[0], bytes[1], bytes[2], bytes[0], bytes[1], bytes[2]);
                numBytes -= 3;
                bytes += 3;
                break;

            case 4:
                LE_INFO(
                    "  %02x %02x %02x %02x | %c %c %c %c",
                    bytes[0], bytes[1], bytes[2], bytes[3], bytes[0], bytes[1], bytes[2], bytes[3]);
                numBytes -= 4;
                bytes += 4;
                break;

            case 5:
                LE_INFO(
                    "  %02x %02x %02x %02x %02x | %c %c %c %c %c",
                    bytes[0], bytes[1], bytes[2], bytes[3], bytes[4],
                    bytes[0], bytes[1], bytes[2], bytes[3], bytes[4]);
                numBytes -= 5;
                bytes += 5;
                break;

            case 6:
                LE_INFO(
                    "  %02x %02x %02x %02x %02x %02x | %c %c %c %c %c %c",
                    bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5],
                    bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5]);
                numBytes -= 6;
                bytes += 6;
                break;

            case 7:
                LE_INFO(
                    "  %02x %02x %02x %02x %02x %02x %02x | %c %c %c %c %c %c %c",
                    bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6],
                    bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6]);
                numBytes -= 7;
                bytes += 7;
                break;

            default:
                LE_INFO(
                    "  %02x %02x %02x %02x %02x %02x %02x %02x | %c %c %c %c %c %c %c %c",
                    bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7],
                    bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7]);
                numBytes -= 8;
                bytes += 8;
                break;
        }
    }
}

static void stdoutHandler(int fd, short events)
{
    static char line[256];
    static size_t numCharsInLineBuffer = 0;
    static size_t cursorPosition = 0;
    static bool processingEscape = false;

    LE_ASSERT(fd == m.pipes.childStdout[PIPE_READ_END_INDEX]);

    if (events & POLLIN)
    {
        uint8_t buffer[1024];
        const ssize_t readResult = read(fd, buffer, sizeof(buffer));

        LE_FATAL_IF(
            readResult <= 0, "Unexpected read result from gatttool stdout pipe (%d)", readResult);
        const size_t numRead = readResult;
        // DEBUG
        //LE_INFO("Read %d bytes from stdout and the bytes are:", numRead);
        //dumpBytes(buffer, numRead);
        // END DEBUG
        for (size_t bufferOffset = 0; bufferOffset < numRead; bufferOffset++)
        {
            LE_FATAL_IF(
                numCharsInLineBuffer == sizeof(line),
                "No room to place more characters in the line buffer");
            if (buffer[bufferOffset] == '\n')
            {
                // Remove any trailing spaces from the string
                for (int i = numCharsInLineBuffer - 1; i != 0; i--)
                {
                    if (line[i] == ' ')
                    {
                        numCharsInLineBuffer--;
                    }
                    else
                    {
                        break;
                    }
                }
                line[numCharsInLineBuffer] = '\0';
                stdoutLineHandler(line, numCharsInLineBuffer);
                numCharsInLineBuffer = 0;
                cursorPosition = 0;
            }
            else if (buffer[bufferOffset] == '\r')
            {
                cursorPosition = 0;
            }
            else if (buffer[bufferOffset] == '\b')
            {
                if (cursorPosition > 0)
                {
                    cursorPosition--;
                }
            }
            else if (buffer[bufferOffset] == 0x1B) // esc
            {
                processingEscape = true;
            }
            else if (processingEscape && buffer[bufferOffset] == 'm')
            {
                processingEscape = false;
            }
            else if (!processingEscape)
            {
                line[cursorPosition++] = buffer[bufferOffset];
                if (cursorPosition > numCharsInLineBuffer)
                {
                    numCharsInLineBuffer = cursorPosition;
                }
            }
        }
    }

    if (events & POLLHUP)
    {
        le_fdMonitor_Delete(m.fdMonitors.childStdout);
        LE_FATAL("Child process has closed the FD for stdout");
    }
}

static void stderrHandler(int fd, short events)
{
    LE_ASSERT(fd == m.pipes.childStderr[PIPE_READ_END_INDEX]);
    char buffer[128] = {0};
    const ssize_t readResult = read(fd, buffer, sizeof(buffer));
    LE_FATAL_IF(
        readResult <= 0, "Unexpected read result from gatttool stderr pipe (%d)", readResult);
    LE_FATAL("An fd notification was received on stderr.  stderr contains: '%s'", buffer);
}

static void childProcess(void)
{
    // Close the original standard file handles of this process
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Close the ends of the pipe which this process does not need
    close(m.pipes.childStdin[PIPE_WRITE_END_INDEX]);
    close(m.pipes.childStdout[PIPE_READ_END_INDEX]);
    close(m.pipes.childStderr[PIPE_READ_END_INDEX]);

    // Copy the file descriptors associated with the pipe into the standard numbers
    dup2(m.pipes.childStdin[PIPE_READ_END_INDEX], STDIN_FILENO);
    dup2(m.pipes.childStdout[PIPE_WRITE_END_INDEX], STDOUT_FILENO);
    dup2(m.pipes.childStderr[PIPE_WRITE_END_INDEX], STDERR_FILENO);

    // Instantiate gatttool
    char deviceArg[32] = "--device=";
    strncat(deviceArg, m.deviceMacAddr, sizeof(deviceArg));
    char* argv[] = {"/legato/systems/current/bin/gatttool", "--interactive", deviceArg, 0};
    int r = execvp(argv[0], argv);
    if (r == -1)
    {
        // error has ocurred.  I don't think we can LE_ASSERT or LE_FATAL because stdout/stderr are
        // already closed.
        exit(1);
    }
}

static void parentProcess(void)
{
    // Close the ends of the pipe which this process does not need
    close(m.pipes.childStdin[PIPE_READ_END_INDEX]);
    close(m.pipes.childStdout[PIPE_WRITE_END_INDEX]);
    close(m.pipes.childStderr[PIPE_WRITE_END_INDEX]);

    m.fdMonitors.childStdin = le_fdMonitor_Create(
        "gatttool stdin",
        m.pipes.childStdin[PIPE_WRITE_END_INDEX],
        stdinHandler,
        POLLERR);
    m.fdMonitors.childStdout = le_fdMonitor_Create(
        "gatttool stdout",
        m.pipes.childStdout[PIPE_READ_END_INDEX],
        stdoutHandler,
        POLLIN | POLLHUP);
    m.fdMonitors.childStderr = le_fdMonitor_Create(
        "gatttool stderr",
        m.pipes.childStderr[PIPE_READ_END_INDEX],
        stderrHandler,
        POLLIN | POLLHUP);

    dataRouter_SessionStart("eu.airvantage.net", "SWI", true, DATAROUTER_CACHE);

    // Initialize the commandResponseTimer to NULL so that the STATE_BEGIN handling in
    // continueInitialization() can distinguish between a fresh start and a connect retry.
    m.initialization.commandResponseTimer = NULL;
    m.programState = STATE_BEGIN;
    continueInitialization();
}

static void logData
(
    const SensorReadings* reading,
    const SensorCalculation* calculation
)
{
    LE_INFO(
        "ir: object temperature=%f, ambient temperature=%f",
        reading->irTemperature.objectTemperatureInCelcius,
        reading->irTemperature.ambientTemperatureInCelcius);
    LE_INFO(
        "humidity: temperature=%f, humidity=%f",
        reading->humidity.temperatureInCelcius,
        reading->humidity.humidityInRelativePercent);
    LE_INFO(
        "barometric pressure: temperature=%f, pressure=%f",
        reading->barometricPressure.temperatureInCelcius,
        reading->barometricPressure.pressureInHectoPascal);
    LE_INFO("Optical: luminosity in lux=%f", reading->optical.luminosityInLux);
    LE_INFO(
        "movement/gyro: x=%f, y=%f, z=%f",
        reading->movement.gyroInDegreesPerSecond.x,
        reading->movement.gyroInDegreesPerSecond.y,
        reading->movement.gyroInDegreesPerSecond.z);
    LE_INFO(
        "movement/acceleration: x=%f, y=%f, z=%f",
        reading->movement.accelerometerInG.x,
        reading->movement.accelerometerInG.y,
        reading->movement.accelerometerInG.z);
    LE_INFO(
        "movement/magnetometer: x=%f, y=%f, z=%f",
        reading->movement.magnetometerInMicroTesla.x,
        reading->movement.magnetometerInMicroTesla.y,
        reading->movement.magnetometerInMicroTesla.z);
    LE_INFO(
        "Calculations: compassAngle=%f, shock=%f, orientation=%d",
        calculation->compassAngle,
        calculation->shock,
        calculation->orientation);
}

static bool approxEq(float v1, float v2, float threshold)
{
    return (fabsf(v1 - v2) < threshold);
}

static void performSensorCalculation(
    SensorCalculation* calculation,
    const SensorReadings* currentReading,
    const SensorReadings* previousReading)
{
    // Compass Angle
    // TODO: Calculation is incorrect.  We must take the orientation into account to know which two
    // axis are the horizontal ones and know what a positive/negative value means.
    calculation->compassAngle = currentReading->movement.magnetometerInMicroTesla.x == 0.0 ?
        0.0 :
        (atan(
            currentReading->movement.magnetometerInMicroTesla.y /
            currentReading->movement.magnetometerInMicroTesla.x) * 360.0) / 3.14;

    // Shock
    const float deltaX =
        currentReading->movement.accelerometerInG.x - previousReading->movement.accelerometerInG.x;
    const float deltaY =
        currentReading->movement.accelerometerInG.y - previousReading->movement.accelerometerInG.y;
    const float deltaZ =
        currentReading->movement.accelerometerInG.z - previousReading->movement.accelerometerInG.z;
    calculation->shock = sqrt((deltaX * deltaX) + (deltaY * deltaY) + (deltaZ * deltaZ));

    // Orientation
    float allowableError = 0.3;
    if (approxEq(currentReading->movement.accelerometerInG.x, 1.0, allowableError) &&
        approxEq(currentReading->movement.accelerometerInG.y, 0.0, allowableError) &&
        approxEq(currentReading->movement.accelerometerInG.z, 0.0, allowableError))
    {
        calculation->orientation = 1;
    }
    else if (approxEq(currentReading->movement.accelerometerInG.x, -1.0, allowableError) &&
        approxEq(currentReading->movement.accelerometerInG.y, 0.0, allowableError) &&
        approxEq(currentReading->movement.accelerometerInG.z, 0.0, allowableError))
    {
        calculation->orientation = 2;
    }
    else if (approxEq(currentReading->movement.accelerometerInG.x, 0.0, allowableError) &&
        approxEq(currentReading->movement.accelerometerInG.y, 1.0, allowableError) &&
        approxEq(currentReading->movement.accelerometerInG.z, 0.0, allowableError))
    {
        calculation->orientation = 3;
    }
    else if (approxEq(currentReading->movement.accelerometerInG.x, 0.0, allowableError) &&
        approxEq(currentReading->movement.accelerometerInG.y, -1.0, allowableError) &&
        approxEq(currentReading->movement.accelerometerInG.z, 0.0, allowableError))
    {
        calculation->orientation = 4;
    }
    else if (approxEq(currentReading->movement.accelerometerInG.x, 0.0, allowableError) &&
        approxEq(currentReading->movement.accelerometerInG.y, 0.0, allowableError) &&
        approxEq(currentReading->movement.accelerometerInG.z, 1.0, allowableError))
    {
        calculation->orientation = 5;
    }
    else if (approxEq(currentReading->movement.accelerometerInG.x, 0.0, allowableError) &&
        approxEq(currentReading->movement.accelerometerInG.y, 0.0, allowableError) &&
        approxEq(currentReading->movement.accelerometerInG.z, -1.0, allowableError))
    {
        calculation->orientation = 6;
    }
    else
    {
        // Unknown
        calculation->orientation = 0;
    }
}

static void updateSensorTagOutputs(void)
{
    // Bits
    //  0 -> Red LED
    //  1 -> Green LED
    //  2 -> Buzzer
    const uint8_t value[] =
    {
        ((m.outputStates.redLedOn ? 1 : 0) << 0) |
        ((m.outputStates.greenLedOn ? 1 : 0) << 1) |
        ((m.outputStates.buzzerOn ? 1 : 0) << 2)
    };
    charWriteCmd(m.handles.io.data, value, sizeof(value));
}

static void outputUpdateHandler(
    dataRouter_DataType_t type,
    const char* key,
    void* context)
{
    LE_FATAL_IF(
        type != DATAROUTER_BOOLEAN,
        "Output update handler received update with unexpected type");
    uint32_t timestamp;
    if (strcmp(key, KEY_RED_LED) == 0)
    {
        dataRouter_ReadBoolean(key, &m.outputStates.redLedOn, &timestamp);
    }
    else if (strcmp(key, KEY_GREEN_LED) == 0)
    {
        dataRouter_ReadBoolean(key, &m.outputStates.greenLedOn, &timestamp);
    }
    else if (strcmp(key, KEY_BUZZER) == 0)
    {
        dataRouter_ReadBoolean(key, &m.outputStates.buzzerOn, &timestamp);
    }
    else
    {
        LE_FATAL("Received update for unexpected key (%s)", key);
    }
    updateSensorTagOutputs();
}

static void sigChldHandler(int signal)
{
    LE_ASSERT(signal == SIGCHLD);
    LE_FATAL("Caught SIGCHLD signal indicating gatttool has exited");
}

COMPONENT_INIT
{
    le_result_t macReadResult = le_cfg_QuickGetString(
        "bleSensorInterface:/sensorMac", m.deviceMacAddr, sizeof(m.deviceMacAddr), "");
    if (macReadResult != LE_OK)
    {
        LE_FATAL("MAC address stored in configuration tree is too big");
    }
    else if (strcmp(m.deviceMacAddr, "") == 0)
    {
        LE_FATAL("MAC address is not stored in bleSensorInterface:/sensorMac");
    }
    else
    {
        // Make sure MAC is uppercase for consistency
        for (char* c = m.deviceMacAddr; *c != '\0'; c++)
        {
            *c = toupper((unsigned char)*c);
        }
        LE_INFO("Read Sensortag MAC address (%s) from configTree", m.deviceMacAddr);
    }

    le_sig_Block(SIGCHLD);
    le_sig_SetEventHandler(SIGCHLD, sigChldHandler);

    LE_FATAL_IF(initPipes() != 0, "Failed to initialize pipes");

    pid_t pid = fork();
    LE_FATAL_IF(pid == -1, "Call to fork() has failed!");

    if (pid == 0)
    {
        childProcess();
    }
    else
    {
        parentProcess();
    }
}
