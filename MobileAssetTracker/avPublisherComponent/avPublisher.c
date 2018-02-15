#include "legato.h"
#include "interfaces.h"
#include "lightSensor.h"
#include "pressureSensor.h"
#include "accelerometer.h"
#include "gps.h"

#define LOCATION_REQUEST_TOKEN 		"932546333b1b11"
#define LOCATION_MAX_CELLS		6
#define LOCATION_MAX_AP			15

#define LIGHT_SENSOR_ENABLE

//--------------------------------------------------------------------------------------------------
/*
 * type definitions
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * An abstract representation of a sensor
 *
 * Values are represented as void* because different sensors may produce double, uint32_t or a
 * custom struct type. The only requirement is that all of the functions must expect the same type
 * of value.
 */
//--------------------------------------------------------------------------------------------------
struct Item
{
    // A human readable name for the sensor
    const char *name;

    // Reads a value from the sensor
    le_result_t (*read)(void *value);

    // Checks to see if the value read exceeds the threshold relative to the last recorded value. If
    // the function returns true, the readValue will be recorded.
    bool (*thresholdCheck)(const void *recordedValue, const void *readValue);

    // Records the value into the given record.
    le_result_t (*record)(le_avdata_RecordRef_t ref, uint64_t timestamp, void *value);

    // Copies the sensor reading from src to dest. The management of the src and dest data is the
    // responsibility of the caller.
    void (*copyValue)(void *dest, const void *src);

    // Most recently read value from the sensor. Should be initialized to point at a variable large
    // enough to store a sensor reading.
    void *lastValueRead;

    // Most recently recorded value from the sensor. Must be initialized to point to a variable to
    // store a reading and must be a differnt variable from what lastValueRead is pointing at.
    void *lastValueRecorded;

    // Time when the last reading was recorded.
    uint64_t lastTimeRecorded;

    // Time when the last reading was read.
    uint64_t lastTimeRead;
};

//--------------------------------------------------------------------------------------------------
/**
 * 3D acceleration value.
 */
//--------------------------------------------------------------------------------------------------
struct Acceleration
{
    double x;
    double y;
    double z;
};

//--------------------------------------------------------------------------------------------------
/**
 * A 3D angular velocity value read from the accelerometer.
 */
//--------------------------------------------------------------------------------------------------
struct Gyro
{
    double x;
    double y;
    double z;
};

struct Location3d
{
    double latitude;
    double longitude;
    double hAccuracy;
    double altitude;
    double vAccuracy;
};

//--------------------------------------------------------------------------------------------------
/**
 * Current location.
 */
//--------------------------------------------------------------------------------------------------
struct Location
{
    char mcc[LE_MRC_MCC_BYTES];
    char mnc[LE_MRC_MNC_BYTES];
    le_mrc_Rat_t rat;
    struct {
	uint32_t cid;
    	uint32_t lac;
	int32_t signal;
    } cellInfo[LOCATION_MAX_CELLS];
    struct {
        uint8_t ssid[LE_WIFIDEFS_MAX_SSID_BYTES];
	char bssid[LE_WIFIDEFS_MAX_BSSID_BYTES];
	size_t ssidLen;
	int16_t signal;
    } wifiInfo[LOCATION_MAX_AP];
    uint32_t numAPs;
    uint32_t numCells;
};

//--------------------------------------------------------------------------------------------------
/**
 * A data structure that stores a single reading from all of the sensors.
 */
//--------------------------------------------------------------------------------------------------
struct SensorReadings
{
#ifdef LIGHT_SENSOR_ENABLE
    int32_t lightLevel;
#endif // LIGHT_SENSOR_ENABLE
    double pressure;
    double temperature;
    struct Acceleration acc;
    struct Gyro gyro;
    struct Location location;

#ifdef GPS_ENABLE
    struct Location3d location3d;
#endif // GPS_ENABLE
};

//--------------------------------------------------------------------------------------------------
/*
 * static function declarations
 */
//--------------------------------------------------------------------------------------------------
static void PushCallbackHandler(le_avdata_PushStatus_t status, void* context);
static uint64_t GetCurrentTimestamp(void);
static void SampleTimerHandler(le_timer_Ref_t timer);

#ifdef LIGHT_SENSOR_ENABLE
static le_result_t LightSensorRead(void *value);
static bool LightSensorThreshold(const void *recordedValue, const void* readValue);
static le_result_t LightSensorRecord(le_avdata_RecordRef_t ref, uint64_t timestamp, void *value);
static void LightSensorCopyValue(void *dest, const void *src);
#endif // LIGHT_SENSOR_ENABLE

static le_result_t PressureSensorRead(void *value);
static bool PressureSensorThreshold(const void *recordedValue, const void* readValue);
static le_result_t PressureSensorRecord(le_avdata_RecordRef_t ref, uint64_t timestamp, void *value);
static void PressureSensorCopyValue(void *dest, const void *src);

static le_result_t TemperatureSensorRead(void *value);
static bool TemperatureSensorThreshold(const void *recordedValue, const void* readValue);
static le_result_t TemperatureSensorRecord(le_avdata_RecordRef_t ref, uint64_t timestamp, void *value);
static void TemperatureSensorCopyValue(void *dest, const void *src);

static le_result_t AccelerometerRead(void *value);
static bool AccelerometerThreshold(const void *recordedValue, const void* readValue);
static le_result_t AccelerometerRecord(le_avdata_RecordRef_t ref, uint64_t timestamp, void *value);
static void AccelerometerCopyValue(void *dest, const void *src);

static le_result_t GyroRead(void *value);
static bool GyroThreshold(const void *recordedValue, const void* readValue);
static le_result_t GyroRecord(le_avdata_RecordRef_t ref, uint64_t timestamp, void *value);
static void GyroCopyValue(void *dest, const void *src);

static le_result_t LocationRead(void *value);
static bool LocationThreshold(const void *recordedValue, const void* readValue);
static le_result_t LocationRecord(le_avdata_RecordRef_t ref, uint64_t timestamp, void *value);
static void LocationCopyValue(void *dest, const void *src);

#ifdef GPS_ENABLE
static le_result_t GpsRead(void *value);
static bool GpsThreshold(const void *recordedValue, const void* readValue);
static le_result_t GpsRecord(le_avdata_RecordRef_t ref, uint64_t timestamp, void *value);
static void GpsCopyValue(void *dest, const void *src);
#endif // GPS_ENABLE

static void AvSessionStateHandler (le_avdata_SessionState_t state, void *context);



//--------------------------------------------------------------------------------------------------
/*
 * variable definitions
 */
//--------------------------------------------------------------------------------------------------

// Wait time between each round of sensor readings.
static const int DelayBetweenReadings = 30;

// The maximum amount of time to wait for a reading to exceed a threshold before a publish is
// forced.
static const int MaxIntervalBetweenPublish = 120;

// The minimum amount of time to wait between publishing data.
static const int MinIntervalBetweenPublish = 30;

// How old the last published value must be for an item to be considered stale. The next time a
// publish occurs, the most recent reading of all stale items will be published.
static const int TimeToStale = 60;

static le_timer_Ref_t SampleTimer;
static le_avdata_RequestSessionObjRef_t AvSession;
static le_avdata_RecordRef_t RecordRef;
static le_avdata_SessionStateHandlerRef_t HandlerRef;

static bool DeferredPublish = false;
static uint64_t LastTimePublished = 0;


//--------------------------------------------------------------------------------------------------
/*
 * Data storage for sensor readings.
 *
 * This struct contains the most recently read values from the sensors and the most recently
 * recorded values from the sensors.
 */
//--------------------------------------------------------------------------------------------------
static struct
{
    struct SensorReadings recorded;  // sensor values most recently recorded
    struct SensorReadings read;      // sensor values most recently read
} SensorData;


//--------------------------------------------------------------------------------------------------
/**
 * An array representing all of the sensor values to read and publish
 */
//--------------------------------------------------------------------------------------------------
struct Item Items[] =
{
#ifdef LIGHT_SENSOR_ENABLE
    {
        .name = "light level",
        .read = LightSensorRead,
        .thresholdCheck = LightSensorThreshold,
        .record = LightSensorRecord,
        .copyValue = LightSensorCopyValue,
        .lastValueRead = &SensorData.read.lightLevel,
        .lastValueRecorded = &SensorData.recorded.lightLevel,
        .lastTimeRead = 0,
        .lastTimeRecorded = 0,
    },
#endif // LIGHT_SENSOR_ENABLE
    {
        .name = "pressure",
        .read = PressureSensorRead,
        .thresholdCheck = PressureSensorThreshold,
        .record = PressureSensorRecord,
        .copyValue = PressureSensorCopyValue,
        .lastValueRead = &SensorData.read.pressure,
        .lastValueRecorded = &SensorData.recorded.pressure,
        .lastTimeRead = 0,
        .lastTimeRecorded = 0,
    },
    {
        .name = "temperature",
        .read = TemperatureSensorRead,
        .thresholdCheck = TemperatureSensorThreshold,
        .record = TemperatureSensorRecord,
        .copyValue = TemperatureSensorCopyValue,
        .lastValueRead = &SensorData.read.temperature,
        .lastValueRecorded = &SensorData.recorded.temperature,
        .lastTimeRead = 0,
        .lastTimeRecorded = 0,
    },
    {
        .name = "accelerometer",
        .read = AccelerometerRead,
        .thresholdCheck = AccelerometerThreshold,
        .record = AccelerometerRecord,
        .copyValue = AccelerometerCopyValue,
        .lastValueRead = &SensorData.read.acc,
        .lastValueRecorded = &SensorData.recorded.acc,
        .lastTimeRead = 0,
        .lastTimeRecorded = 0,
    },
    {
        .name = "gyro",
        .read = GyroRead,
        .thresholdCheck = GyroThreshold,
        .record = GyroRecord,
        .copyValue = GyroCopyValue,
        .lastValueRead = &SensorData.read.gyro,
        .lastValueRecorded = &SensorData.recorded.gyro,
        .lastTimeRead = 0,
        .lastTimeRecorded = 0,
    },
    {
        .name = "location",
        .read = LocationRead,
        .thresholdCheck = LocationThreshold,
        .record = LocationRecord,
        .copyValue = LocationCopyValue,
	.lastValueRead = &SensorData.read.location,
        .lastValueRecorded = &SensorData.recorded.location,
        .lastTimeRead = 0,
        .lastTimeRecorded = 0,
    },
#ifdef GPS_ENABLE
    {
        .name = "gps",
        .read = GpsRead,
        .thresholdCheck = GpsThreshold,
        .record = GpsRecord,
        .copyValue = GpsCopyValue,
        .lastValueRead = &SensorData.read.location3d,
        .lastValueRecorded = &SensorData.recorded.location3d,
        .lastTimeRead = 0,
        .lastTimeRecorded = 0,
    },
#endif // GPS_ENABLE
};


//--------------------------------------------------------------------------------------------------
/*
 * static function definitions
 */
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Handles notification of LWM2M push status.
 *
 * This function will warn if there is an error in pushing data, but it does not make any attempt to
 * retry pushing the data.
 */
//--------------------------------------------------------------------------------------------------
static void PushCallbackHandler
(
    le_avdata_PushStatus_t status, ///< Push success/failure status
    void* context                  ///< Not used
)
{
    switch (status)
    {
    case LE_AVDATA_PUSH_SUCCESS:
        // data pushed successfully
	LE_INFO("Push was successful");
        break;

    case LE_AVDATA_PUSH_FAILED:
        LE_WARN("Push was not successful");
        break;

    default:
        LE_ERROR("Unhandled push status %d", status);
        break;
    }

    le_avdata_DeleteRecord(RecordRef);
    RecordRef = NULL;

    le_result_t result = le_timer_Start(SampleTimer);
    if (result == LE_BUSY)
    {
        LE_INFO("Timer was already running");
    }
    else
    {
        LE_ASSERT_OK(result);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Convenience function to get current time as uint64_t.
 *
 * @return
 *      Current time as a uint64_t
 */
//--------------------------------------------------------------------------------------------------
static uint64_t GetCurrentTimestamp(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t utcMilliSec = (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;
    return utcMilliSec;
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler for the sensor sampling timer
 *
 * Each time this function is called due to timer expiry each sensor described in the Items array
 * will be read. If any sensor item's thresholdCheck() function returns true, then that reading is
 * recorded and a publish action is scheduled. The data will be published immediately unless fewer
 * than MinIntervalBetweenPublish seconds have elapsed since the last publish. If that is the case,
 * the publish will be deferred until the minimum wait has elapsed. If no publish has occurred for
 * MaxIntervalBetweenPublish seconds, then a publish is forced. When a push is about to be executed
 * the list of items is checked again for any entries which have not been recorded in greater than
 * TimeToStale seconds. Stale items are recorded and then the record is published.
 */
//--------------------------------------------------------------------------------------------------
static void SampleTimerHandler
(
    le_timer_Ref_t timer  ///< Sensor sampling timer
)
{
    uint64_t now = GetCurrentTimestamp();
    bool publish = false;

    if (!RecordRef) 
    {
	RecordRef = le_avdata_CreateRecord();
    }

    for (int i = 0; i < NUM_ARRAY_MEMBERS(Items); i++)
    {
        le_result_t r;
        struct Item *it = &Items[i];
        r = it->read(it->lastValueRead);
        if (r == LE_OK)
        {
            it->lastTimeRead = now;
            if (it->lastTimeRecorded == 0 || it->thresholdCheck(it->lastValueRead, it->lastValueRecorded))
            {
                r = it->record(RecordRef, now, it->lastValueRead);
                if (r == LE_OK)
                {
                    it->copyValue(it->lastValueRecorded, it->lastValueRead);
                    publish = true;
                }
                else
                {
                    LE_WARN("Failed to record %s", it->name);
                }
            }
        }
        else
        {
            LE_WARN("Failed to read %s", it->name);
        }

        if ((now - it->lastTimeRecorded) > (MaxIntervalBetweenPublish * 1000) &&
            it->lastTimeRead > LastTimePublished)
        {
            publish = true;
        }
    }

    LE_DEBUG("publish(%d) DeferredPublish(%d)", publish, DeferredPublish);
    if (publish || DeferredPublish)
    {
        if ((now - LastTimePublished) < MinIntervalBetweenPublish)
        {
            DeferredPublish = true;

	    le_result_t result = le_timer_Start(SampleTimer);
    	    if (result == LE_BUSY)
    	    {
            	LE_INFO("Timer was already running");
            }
            else
            {
            	LE_ASSERT_OK(result);
            }
        }
        else
        {
            // Find all of the stale items and record their current reading
            for (int i = 0; i < NUM_ARRAY_MEMBERS(Items); i++)
            {
                struct Item *it = &Items[i];
                if ((now - it->lastTimeRecorded) > (TimeToStale * 1000) &&
                    it->lastTimeRead > it->lastTimeRecorded)
                {
                    le_result_t r = it->record(RecordRef, it->lastTimeRead, it->lastValueRead);
                    if (r == LE_OK)
                    {
                        it->copyValue(it->lastValueRecorded, it->lastValueRead);
                        it->lastTimeRecorded = it->lastTimeRead;
                    }
                    else
                    {
                        LE_WARN("Failed to record %s", it->name);
                    }
                }
            }

	    LE_DEBUG("Push record");
            le_result_t r = le_avdata_PushRecord(RecordRef, PushCallbackHandler, NULL);
            if (r != LE_OK)
            {
		if (r == LE_BUSY) 
		{
		    LE_WARN("Push record - %s", LE_RESULT_TXT(r));
		}
                else 
		{
		    LE_ERROR("Failed to push record - %s", LE_RESULT_TXT(r));

		    r = le_timer_Start(SampleTimer);
    		    if (r == LE_BUSY)
    		    {
                        LE_INFO("Timer was already running");
        	    }
        	    else
        	    {
            		LE_ASSERT_OK(r);
        	    }
		}
            }
            else
            {
		LE_DEBUG("Record pushed");
                LastTimePublished = now;
                DeferredPublish = false;
            }
        }
    }
    else
    {
        le_result_t result = le_timer_Start(SampleTimer);
    	if (result == LE_BUSY)
    	{
            LE_INFO("Timer was already running");
        }
        else
        {
            LE_ASSERT_OK(result);
        }
    }
}

#ifdef LIGHT_SENSOR_ENABLE
//--------------------------------------------------------------------------------------------------
/**
 * Read the light sensor
 *
 * @return
 *      LE_OK on success.  Any other return value is a failure.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t LightSensorRead
(
    void *value  ///< Pointer to the int32_t variable to store the reading in
)
{
    int32_t *v = value;
    return mangOH_ReadLightSensor(v);
}

//--------------------------------------------------------------------------------------------------
/**
 * Checks to see if the light level has changed sufficiently to warrant recording of a new reading.
 *
 * @return
 *      true if the threshold for recording has been exceeded
 */
//--------------------------------------------------------------------------------------------------
static bool LightSensorThreshold
(
    const void *recordedValue, ///< Last recorded light sensor reading
    const void *readValue      ///< Most recent light sensor reading
)
{
    const int32_t *v1 = recordedValue;
    const int32_t *v2 = readValue;

    return abs(*v1 - *v2) > 200;
}

//--------------------------------------------------------------------------------------------------
/**
 * Records a light sensor reading at the given time into the given record
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the record is full
 *      - LE_FAULT non-specific failure
 */
//--------------------------------------------------------------------------------------------------
static le_result_t LightSensorRecord
(
    le_avdata_RecordRef_t ref, ///< Record reference to record the value into
    uint64_t timestamp,        ///< Timestamp to associate with the value
    void *value                ///< The int32_t value to record
)
{
    const char *path = "Sensors.Light.Level";
    int32_t *v = value;
    le_result_t result = le_avdata_RecordInt(RecordRef, path, *v, timestamp);
    if (result != LE_OK)
    {
        LE_ERROR("Couldn't record light sensor reading - %s", LE_RESULT_TXT(result));
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Copies an int32_t light sensor reading between two void pointers
 */
//--------------------------------------------------------------------------------------------------
static void LightSensorCopyValue
(
    void *dest,      ///< copy destination
    const void *src  ///< copy source
)
{
    int32_t *d = dest;
    const int32_t *s = src;
    *d = *s;
}
#endif // LIGHT_SENSOR_ENABLE

//--------------------------------------------------------------------------------------------------
/**
 * Read the pressure sensor
 *
 * @return
 *      LE_OK on success.  Any other return value is a failure.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t PressureSensorRead
(
    void *value  ///< Pointer to a double to store the reading in
)
{
    double *v = value;
    return mangOH_ReadPressureSensor(v);
}

//--------------------------------------------------------------------------------------------------
/**
 * Checks to see if the pressure has changed sufficiently to warrant recording of a new reading.
 *
 * @return
 *      true if the threshold for recording has been exceeded
 */
//--------------------------------------------------------------------------------------------------
static bool PressureSensorThreshold
(
    const void *recordedValue, ///< Last recorded pressure reading
    const void *readValue      ///< Most recent pressure reading
)
{
    const double *v1 = recordedValue;
    const double *v2 = readValue;

    return fabs(*v1 - *v2) > 1.0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Records a pressure sensor reading at the given time into the given record
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the record is full
 *      - LE_FAULT non-specific failure
 */
//--------------------------------------------------------------------------------------------------
static le_result_t PressureSensorRecord
(
    le_avdata_RecordRef_t ref, ///< Record reference to record the value into
    uint64_t timestamp,        ///< Timestamp to associate with the value
    void *value                ///< The double value to record
)
{
    const char *path = "Sensors.Pressure.Pressure";
    double *v = value;
    le_result_t result = le_avdata_RecordFloat(RecordRef, path, *v, timestamp);
    if (result != LE_OK)
    {
        LE_ERROR("Couldn't record pressure sensor reading - %s", LE_RESULT_TXT(result));
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Copies a double pressure reading between two void pointers
 */
//--------------------------------------------------------------------------------------------------
static void PressureSensorCopyValue
(
    void *dest,      ///< copy destination
    const void *src  ///< copy source
)
{
    double *d = dest;
    const double *s = src;
    *d = *s;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the temperature sensor
 *
 * @return
 *      LE_OK on success.  Any other return value is a failure.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t TemperatureSensorRead
(
    void *value  ///< Pointer to the double variable to store the reading in
)
{
    double *v = value;
    return mangOH_ReadTemperatureSensor(v);
}

//--------------------------------------------------------------------------------------------------
/**
 * Checks to see if the temperature has changed sufficiently to warrant recording of a new reading.
 *
 * @return
 *      true if the threshold for recording has been exceeded
 */
//--------------------------------------------------------------------------------------------------
static bool TemperatureSensorThreshold
(
    const void *recordedValue, ///< Last recorded temperature reading
    const void *readValue      ///< Most recent temperature reading
)
{
    const double *v1 = recordedValue;
    const double *v2 = readValue;

    return fabs(*v1 - *v2) > 2.0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Records a temperature reading at the given time into the given record
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the record is full
 *      - LE_FAULT non-specific failure
 */
//--------------------------------------------------------------------------------------------------
static le_result_t TemperatureSensorRecord
(
    le_avdata_RecordRef_t ref, ///< Record reference to record the value into
    uint64_t timestamp,        ///< Timestamp to associate with the value
    void *value                ///< The double value to record
)
{
    const char *path = "Sensors.Pressure.Temperature";
    double *v = value;
    le_result_t result = le_avdata_RecordFloat(RecordRef, path, *v, timestamp);
    if (result != LE_OK)
    {
        LE_ERROR("Couldn't record pressure sensor reading - %s", LE_RESULT_TXT(result));
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Copies an double temperature reading between two void pointers
 */
//--------------------------------------------------------------------------------------------------
static void TemperatureSensorCopyValue
(
    void *dest,
    const void *src
)
{
    double *d = dest;
    const double *s = src;
    *d = *s;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the acceleration from the accelerometer
 *
 * @return
 *      LE_OK on success.  Any other return value is a failure.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t AccelerometerRead
(
    void *value  ///< Pointer to a struct Acceleration to store the reading in
)
{
    struct Acceleration *v = value;
    return mangOH_ReadAccelerometer(&v->x, &v->y, &v->z);
}

//--------------------------------------------------------------------------------------------------
/**
 * Checks to see if the acceleration has changed sufficiently to warrant recording of a new reading.
 *
 * @return
 *      true if the threshold for recording has been exceeded
 */
//--------------------------------------------------------------------------------------------------
static bool AccelerometerThreshold
(
    const void *recordedValue, ///< Last recorded acceleration reading
    const void *readValue      ///< Most recent acceleration reading
)
{
    const struct Acceleration *v1 = recordedValue;
    const struct Acceleration *v2 = readValue;

    double deltaX = v1->x - v2->x;
    double deltaY = v1->y - v2->y;
    double deltaZ = v1->z - v2->z;

    double deltaAcc = sqrt(pow(deltaX, 2) + pow(deltaY, 2) + pow(deltaZ, 2));

    // The acceleration is in m/s^2, so 9.8 is one G.
    return fabs(deltaAcc) > 1.0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Records an acceleration at the given time into the given record
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the record is full
 *      - LE_FAULT non-specific failure
 */
//--------------------------------------------------------------------------------------------------
static le_result_t AccelerometerRecord
(
    le_avdata_RecordRef_t ref, ///< Record reference to record the value into
    uint64_t timestamp,        ///< Timestamp to associate with the value
    void *value                ///< The struct Acceleration value to record
)
{
    // The '_' is a placeholder that will be replaced
    char path[] = "Sensors.Accelerometer.Acceleration._";
    struct Acceleration *v = value;
    int end = strnlen(path, sizeof(path));
    le_result_t result = LE_FAULT;

    path[end] = 'X';
    LE_INFO("path('%s')", path);
    result = le_avdata_RecordFloat(RecordRef, path, v->x, timestamp);
    if (result != LE_OK)
    {
        LE_ERROR("Couldn't record accelerometer x reading - %s", LE_RESULT_TXT(result));
        goto done;
    }

    path[end] = 'Y';
    LE_INFO("path('%s')", path);
    result = le_avdata_RecordFloat(RecordRef, path, v->y, timestamp);
    if (result != LE_OK)
    {
        LE_ERROR("Couldn't record accelerometer y reading - %s", LE_RESULT_TXT(result));
        goto done;
    }

    path[end] = 'Z';
    LE_INFO("path('%s')", path);
    result = le_avdata_RecordFloat(RecordRef, path, v->z, timestamp);
    if (result != LE_OK)
    {
        LE_ERROR("Couldn't record accelerometer z reading - %s", LE_RESULT_TXT(result));
        goto done;
    }

done:
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Copies a struct Acceleration between two void pointers
 */
//--------------------------------------------------------------------------------------------------
static void AccelerometerCopyValue
(
    void *dest,      ///< copy destination
    const void *src  ///< copy source
)
{
    struct Acceleration *d = dest;
    const struct Acceleration *s = src;
    d->x = s->x;
    d->y = s->y;
    d->z = s->z;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the location
 *
 * @return
 *      LE_OK on success.  Any other return value is a failure.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t LocationRead
(
    void *value  ///< Pointer to a struct Location to store the reading in
)
{
    struct Location *v = value;
    le_mrc_NeighborCellsRef_t ngbrRef;
    le_mrc_CellInfoRef_t cellRef;
    le_mrc_MetricsRef_t metricsRef;
    le_result_t res = LE_OK;

    res = le_mrc_GetCurrentNetworkMccMnc(v->mcc, sizeof(v->mcc), v->mnc, sizeof(v->mnc));	
    if (res != LE_OK) 
    {
        LE_ERROR("ERROR: le_mrc_GetCurrentNetworkMccMnc() failed(%d)", res);
//        goto cleanup;
    }

    res = le_mrc_GetRadioAccessTechInUse(&v->rat);
    if (res != LE_OK) 
    {
        LE_ERROR("ERROR: le_mrc_GetRadioAccessTechInUse() failed(%d)", res);
//        goto cleanup;
    }

    LE_INFO("RAT(0x%02x) MCC/MNC('%s'/'%s')", v->rat, v->mcc, v->mnc);

    v->numCells = 0;
    v->cellInfo[v->numCells].cid = le_mrc_GetServingCellId();
    v->cellInfo[v->numCells].lac = le_mrc_GetServingCellLocAreaCode();

    metricsRef = le_mrc_MeasureSignalMetrics();
    if (metricsRef)
    {
        switch (v->rat) 
        {
        case LE_MRC_RAT_GSM:
        {
            int32_t rssi;
            uint32_t ber;
	    res = le_mrc_GetGsmSignalMetrics(metricsRef, &rssi, &ber);
            if (res != LE_OK) 
            {
                LE_ERROR("ERROR: le_mrc_GetGsmSignalMetrics() failed(%d)", res);
            }
            else
                v->cellInfo[v->numCells].signal = rssi;
            break;
        }
        case LE_MRC_RAT_UMTS:
        {
            int32_t ss;
            uint32_t bler;
            int32_t ecio;
            int32_t rscp;
            int32_t sinr;
            res = le_mrc_GetUmtsSignalMetrics(metricsRef, &ss, &bler, &ecio, &rscp, &sinr);
            if (res != LE_OK) 
            {
                LE_ERROR("ERROR: le_mrc_GetUmtsSignalMetrics() failed(%d)", res);
            }
            else
                v->cellInfo[v->numCells].signal = ss;
            break;
        }
        case LE_MRC_RAT_LTE:
        {
            int32_t ss;
            uint32_t bler; 
            int32_t rsrq; 
            int32_t rsrp; 
            int32_t sinr;
            res = le_mrc_GetLteSignalMetrics(metricsRef, &ss, &bler, &rsrq, &rsrp, &sinr);
            if (res != LE_OK) 
            {
                LE_ERROR("ERROR: le_mrc_GetLteSignalMetrics() failed(%d)", res);
            }
            else
                v->cellInfo[v->numCells].signal = ss;
            break;
        }
        case LE_MRC_RAT_CDMA:
        {
            int32_t ss; 
            uint32_t er; 
            int32_t ecio; 
            int32_t sinr; 
            int32_t io;
            res = le_mrc_GetCdmaSignalMetrics(metricsRef, &ss, &er, &ecio, &sinr, &io);
            if (res != LE_OK) 
            {
                LE_ERROR("ERROR: le_mrc_GetLteSignalMetrics() failed(%d)", res);
            }
            else
                v->cellInfo[v->numCells].signal = ss;
            break;
        }
        default:
            LE_WARN("WARNING: unhandled RAT(%d)", v->rat);
            break;
        }

        le_mrc_DeleteSignalMetrics(metricsRef);
    }

    LE_INFO("%d - cellId(%d) lac(%d)", v->numCells + 1, v->cellInfo[v->numCells].cid, v->cellInfo[v->numCells].lac);
    v->numCells++;

    ngbrRef = le_mrc_GetNeighborCellsInfo();
    if (ngbrRef)
    {
        cellRef = le_mrc_GetFirstNeighborCellInfo(ngbrRef);
        if (cellRef)
        {
            do
            {
                v->cellInfo[v->numCells].cid = le_mrc_GetNeighborCellId(cellRef);
		v->cellInfo[v->numCells].lac = le_mrc_GetNeighborCellLocAreaCode(cellRef);
		v->cellInfo[v->numCells].signal = le_mrc_GetNeighborCellRxLevel(cellRef);
		LE_INFO("%d - signal(%d) cellId(%d) lac(%d)", 
		    v->numCells + 1, v->cellInfo[v->numCells].signal, v->cellInfo[v->numCells].cid, v->cellInfo[v->numCells].lac);
		v->numCells++;
                cellRef = le_mrc_GetNextNeighborCellInfo(ngbrRef);
            } while ((cellRef != NULL) && (v->numCells < LOCATION_MAX_CELLS));
        }

        le_mrc_DeleteNeighborCellsInfo(ngbrRef);
    }

    v->numAPs = 0;
    le_wifiClient_AccessPointRef_t accessPointRef = le_wifiClient_GetFirstAccessPoint();
    while ((NULL != accessPointRef) && (v->numAPs < LOCATION_MAX_AP))
    {
	res = le_wifiClient_GetSsid(accessPointRef, v->wifiInfo[v->numAPs].ssid, &v->wifiInfo[v->numAPs].ssidLen);
    	if (res != LE_OK) 
    	{
            LE_ERROR("ERROR: le_wifiClient_GetSsid() failed(%d)", res);
//            goto cleanup;
    	}

	res = le_wifiClient_GetBssid(accessPointRef, v->wifiInfo[v->numAPs].bssid, sizeof(v->wifiInfo[v->numAPs].bssid));
    	if (res != LE_OK) 
    	{
            LE_ERROR("ERROR: le_wifiClient_GetBssid() failed(%d)", res);
//            goto cleanup;
    	}

	v->wifiInfo[v->numAPs].signal = le_wifiClient_GetSignalStrength(accessPointRef);

        LE_INFO("SSID('%.*s') BSSID('%s') signal(%d)", 
	    v->wifiInfo[v->numAPs].ssidLen, v->wifiInfo[v->numAPs].ssid, 
	    v->wifiInfo[v->numAPs].bssid, v->wifiInfo[v->numAPs].signal);
        accessPointRef = le_wifiClient_GetNextAccessPoint();
	v->numAPs++;
    }

//cleanup:
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Checks to see if the location has changed sufficiently to warrant recording of a new
 * reading.
 *
 * @return
 *      true if the threshold for recording has been exceeded
 */
//--------------------------------------------------------------------------------------------------
static bool LocationThreshold
(
    const void *recordedValue, ///< Last recorded location
    const void *readValue      ///< Most recent location
)
{
    const struct Location *v1 = recordedValue;
    const struct Location *v2 = readValue;
    
    if (strncmp(v1->mcc, v2->mcc, sizeof(v1->mcc))) return true;
    else if (strncmp(v1->mnc, v2->mnc, sizeof(v1->mnc))) return true;
    else if (v1->numCells != v2->numCells) return true;
    else if (v1->numAPs != v2->numAPs) return true;
    else 
    {
        uint32_t i = 0;
        while (i < v1->numCells) 
	{
	    if ((v1->cellInfo[i].cid != v2->cellInfo[i].cid) ||
		(v1->cellInfo[i].lac != v2->cellInfo[i].lac) ||
		(v1->cellInfo[i].signal != v2->cellInfo[i].signal))
		return true;
	    i++;
	}

	i = 0;
	while (i < v1->numAPs) 
	{
	    if ((v1->wifiInfo[i].signal != v2->wifiInfo[i].signal) ||
		(strncmp(v1->wifiInfo[i].bssid, v2->wifiInfo[i].bssid, sizeof(v1->wifiInfo[i].bssid))) ||
		(v1->wifiInfo[i].ssidLen != v2->wifiInfo[i].ssidLen) ||
		(memcmp(v1->wifiInfo[i].ssid, v2->wifiInfo[i].ssid, v1->wifiInfo[i].ssidLen))) 
		return true;
	    i++;
	}
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Records location at the given time into the given record
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the record is full
 *      - LE_FAULT non-specific failure
 */
//--------------------------------------------------------------------------------------------------
static le_result_t LocationRecord
(
    le_avdata_RecordRef_t ref, ///< Record reference to record the value into
    uint64_t timestamp,        ///< Timestamp to associate with the value
    void *value                ///< The struct Gyro value to record
)
{
    struct Location *v = value;
    le_result_t result = LE_FAULT;
    uint32_t i = 0;

    if (strlen(v->mcc) > 0) 
    {
        result = le_avdata_RecordString(RecordRef, "Sensors.Location.CellInfo.MCC", v->mcc, timestamp);
        if (result != LE_OK)
        {
            LE_ERROR("Couldn't record location cell ID reading - %s", LE_RESULT_TXT(result));
            goto done;
        }
    }

    if (strlen(v->mnc) > 0) 
    {
        result = le_avdata_RecordString(RecordRef, "Sensors.Location.CellInfo.MNC", v->mnc, timestamp);
        if (result != LE_OK)
        {
            LE_ERROR("Couldn't record location cell ID reading - %s", LE_RESULT_TXT(result));
            goto done;
        }
    }
    
    while (i < v->numCells) 
    {
	char node[128];

	if (v->cellInfo[i].cid != -1)
	{
	    snprintf(node, sizeof(node), "Sensors.Location.CellInfo.%d.Cid", i + 1);
            LE_INFO("node('%s')", node);
            result = le_avdata_RecordInt(RecordRef, node, v->cellInfo[i].cid, timestamp);
            if (result != LE_OK)
            {
                LE_ERROR("Couldn't record location cell ID reading - %s", LE_RESULT_TXT(result));
                goto done;
            }
	}

	if ((v->cellInfo[i].lac != UINT16_MAX) && (v->cellInfo[i].lac != -1))
	{
	    snprintf(node, sizeof(node), "Sensors.Location.CellInfo.%d.Lac", i + 1);
            LE_INFO("node('%s')", node);
            result = le_avdata_RecordInt(RecordRef, node, v->cellInfo[i].lac, timestamp);
            if (result != LE_OK)
            {
                LE_ERROR("Couldn't record location LAC reading - %s", LE_RESULT_TXT(result));
                goto done;
            }
	}

	if (v->cellInfo[i].signal)
	{
	    snprintf(node, sizeof(node), "Sensors.Location.CellInfo.%d.Signal", i + 1);
            LE_INFO("node('%s')", node);
            result = le_avdata_RecordInt(RecordRef, node, v->cellInfo[i].signal, timestamp);
            if (result != LE_OK)
            {
                LE_ERROR("Couldn't record location signal reading - %s", LE_RESULT_TXT(result));
                goto done;
            }
	}

	i++;
    }

    i = 0;
    while (i < v->numAPs) 
    {
	char node[128];

	snprintf(node, sizeof(node), "Sensors.Location.WiFi.%d.Ssid", i + 1);
        LE_INFO("node('%s')", node);
    	result = le_avdata_RecordString(RecordRef, node, (const char*)v->wifiInfo[i].ssid, timestamp);
    	if (result != LE_OK)
    	{
            LE_ERROR("Couldn't record location Bssid reading - %s", LE_RESULT_TXT(result));
            goto done;
    	}

	snprintf(node, sizeof(node), "Sensors.Location.WiFi.%d.Bssid", i + 1);
        LE_INFO("node('%s')", node);
    	result = le_avdata_RecordString(RecordRef, node, v->wifiInfo[i].bssid, timestamp);
    	if (result != LE_OK)
    	{
            LE_ERROR("Couldn't record location Bssid reading - %s", LE_RESULT_TXT(result));
            goto done;
    	}

	snprintf(node, sizeof(node), "Sensors.Location.WiFi.%d.Signal", i + 1);
        LE_INFO("node('%s')", node);
    	result = le_avdata_RecordInt(RecordRef, node, v->wifiInfo[i].signal, timestamp);
    	if (result != LE_OK)
    	{
            LE_ERROR("Couldn't record location signal reading - %s", LE_RESULT_TXT(result));
            goto done;
    	}

	i++;
    }

done:
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Copies a struct Location between two void pointers
 */
//--------------------------------------------------------------------------------------------------
static void LocationCopyValue
(
    void *dest,      ///< copy destination
    const void *src  ///< copy source
)
{
    struct Location *d = dest;
    const struct Location *s = src;
    uint32_t i = 0;
    
    strncpy(d->mcc, s->mcc, sizeof(d->mcc));
    strncpy(d->mnc, s->mnc, sizeof(d->mnc));

    d->numCells = s->numCells;
    while (i < d->numCells)
    {
        d->cellInfo[i].cid = s->cellInfo[i].cid;
        d->cellInfo[i].lac = s->cellInfo[i].lac;
	i++;
    }

    i = 0;
    d->numAPs = s->numAPs;
    while (i < d->numAPs)
    {
	d->wifiInfo[i].ssidLen = s->wifiInfo[i].ssidLen;
	memcpy(d->wifiInfo[i].ssid, s->wifiInfo[i].ssid, s->wifiInfo[i].ssidLen);
        strncpy(d->wifiInfo[i].bssid, s->wifiInfo[i].bssid, sizeof(d->wifiInfo[i].bssid));
	d->wifiInfo[i].signal = s->wifiInfo[i].signal;
	i++;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the angular velocity from the accelerometer
 *
 * @return
 *      LE_OK on success.  Any other return value is a failure.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GyroRead
(
    void *value  ///< Pointer to a struct Gyro to store the reading in
)
{
    struct Gyro *v = value;
    return mangOH_ReadGyro(&v->x, &v->y, &v->z);
}

//--------------------------------------------------------------------------------------------------
/**
 * Checks to see if the angular velocity has changed sufficiently to warrant recording of a new
 * reading.
 *
 * @return
 *      true if the threshold for recording has been exceeded
 */
//--------------------------------------------------------------------------------------------------
static bool GyroThreshold
(
    const void *recordedValue, ///< Last recorded angular velocity
    const void *readValue      ///< Most recent angular velocity
)
{
    const struct Gyro *v1 = recordedValue;
    const struct Gyro *v2 = readValue;

    double deltaX = v1->x - v2->x;
    double deltaY = v1->y - v2->y;
    double deltaZ = v1->z - v2->z;

    double deltaAngVel = sqrt(pow(deltaX, 2) + pow(deltaY, 2) + pow(deltaZ, 2));

    return fabs(deltaAngVel) > (M_PI / 2.0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Records an angular velocity at the given time into the given record
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the record is full
 *      - LE_FAULT non-specific failure
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GyroRecord
(
    le_avdata_RecordRef_t ref, ///< Record reference to record the value into
    uint64_t timestamp,        ///< Timestamp to associate with the value
    void *value                ///< The struct Gyro value to record
)
{
    // The '_' is a placeholder that will be replaced
    char path[] = "Sensors.Accelerometer.Gyro.";
    struct Gyro *v = value;
    int end = strnlen(path, sizeof(path));
    le_result_t result = LE_FAULT;

    path[end] = 'X';
    LE_INFO("path('%s')", path);
    result = le_avdata_RecordFloat(RecordRef, path, v->x, timestamp);
    if (result != LE_OK)
    {
        LE_ERROR("Couldn't record gyro x reading - %s", LE_RESULT_TXT(result));
        goto done;
    }

    path[end] = 'Y';
    LE_INFO("path('%s')", path);
    result = le_avdata_RecordFloat(RecordRef, path, v->y, timestamp);
    if (result != LE_OK)
    {
        LE_ERROR("Couldn't record gyro y reading - %s", LE_RESULT_TXT(result));
        goto done;
    }

    path[end] = 'Z';
    LE_INFO("path('%s')", path);
    result = le_avdata_RecordFloat(RecordRef, path, v->z, timestamp);
    if (result != LE_OK)
    {
        LE_ERROR("Couldn't record gyro z reading - %s", LE_RESULT_TXT(result));
        goto done;
    }

done:
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Copies a struct Gyro between two void pointers
 */
//--------------------------------------------------------------------------------------------------
static void GyroCopyValue
(
    void *dest,      ///< copy destination
    const void *src  ///< copy source
)
{
    struct Gyro *d = dest;
    const struct Gyro *s = src;
    d->x = s->x;
    d->y = s->y;
    d->z = s->z;
}

#ifdef GPS_ENABLE
//--------------------------------------------------------------------------------------------------
/**
 * Read the GPS location
 *
 * @return
 *      LE_OK on success.  Any other return value is a failure.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GpsRead
(
    void *value  ///< Pointer to a struct Location3d to store the reading in
)
{
    struct Location3d *v = value;
    return mangOH_ReadGps(&v->latitude, &v->longitude, &v->hAccuracy, &v->altitude, &v->vAccuracy);
}

//--------------------------------------------------------------------------------------------------
/**
 * Checks to see if the location has changed sufficiently to warrant recording of a new reading.
 *
 * @return
 *      true if the threshold for recording has been exceeded
 */
//--------------------------------------------------------------------------------------------------
static bool GpsThreshold
(
    const void *recordedValue, ///< Last recorded angular velocity
    const void *readValue      ///< Most recent angular velocity
)
{
    const struct Location3d *v1 = recordedValue;
    const struct Location3d *v2 = readValue;

    double deltaLat = v2->latitude - v1->latitude;
    double deltaLon = v2->longitude - v1->longitude;
    /*
    double deltaHAccuracy = v2->hAccuracy - v1->hAccuracy;
    double deltaAltitude = v2->altitude - v1->altitude;
    double deltaVAccuracy = v2->vAccuracy - v1->vAccuracy;
    */

    // TODO: It makes sense to publish a new value if the possible position of the device has
    // changed by certain number of meters. Calculating the number of meters between two latitudes
    // or two longitudes requires complicated math.  Just doing something dumb for now.

    return fabs(deltaLat) + fabs(deltaLon) > 0.01;
}

//--------------------------------------------------------------------------------------------------
/**
 * Records a GPS reading at the given time into the given record
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the record is full
 *      - LE_FAULT non-specific failure
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GpsRecord
(
    le_avdata_RecordRef_t ref, ///< Record reference to record the value into
    uint64_t timestamp,        ///< Timestamp to associate with the value
    void *value                ///< The struct Gps value to record
)
{
    char path[128] = "lwm2m.6.0.";
    int end = strnlen(path, sizeof(path));
    struct Location3d *v = value;
    le_result_t result = LE_FAULT;

    strcpy(&path[end], "0");
    LE_INFO("path('%s')", path);
    result = le_avdata_RecordFloat(RecordRef, path, v->latitude, timestamp);
    if (result != LE_OK)
    {
        LE_ERROR("Couldn't record gps latitude reading - %s", LE_RESULT_TXT(result));
        goto done;
    }

    strcpy(&path[end], "1");
    LE_INFO("path('%s')", path);
    result = le_avdata_RecordFloat(RecordRef, path, v->longitude, timestamp);
    if (result != LE_OK)
    {
        LE_ERROR("Couldn't record gps longitude reading - %s", LE_RESULT_TXT(result));
        goto done;
    }

    strcpy(&path[end], "3");
    LE_INFO("path('%s')", path);
    result = le_avdata_RecordFloat(RecordRef, path, v->hAccuracy, timestamp);
    if (result != LE_OK)
    {
        LE_ERROR("Couldn't record gps horizontal accuracy reading - %s", LE_RESULT_TXT(result));
        goto done;
    }

    strcpy(&path[end], "2");
    LE_INFO("path('%s')", path);
    result = le_avdata_RecordFloat(RecordRef, path, v->altitude, timestamp);
    if (result != LE_OK)
    {
        LE_ERROR("Couldn't record gps altitude reading - %s", LE_RESULT_TXT(result));
        goto done;
    }

    result = le_avdata_RecordFloat(RecordRef, "Sensors.Gps.VerticalAccuracy", v->vAccuracy, timestamp);
    if (result != LE_OK)
    {
        LE_ERROR("Couldn't record gps vertical accuracy reading - %s", LE_RESULT_TXT(result));
        goto done;
    }

done:
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Copies a struct Location3d between two void pointers
 */
//--------------------------------------------------------------------------------------------------
static void GpsCopyValue
(
    void *dest,      ///< copy destination
    const void *src  ///< copy source
)
{
    struct Location3d *d = dest;
    const struct Location3d *s = src;
    d->latitude = s->latitude;
    d->longitude = s->longitude;
    d->hAccuracy = s->hAccuracy;
    d->altitude = s->altitude;
    d->vAccuracy = s->vAccuracy;
}
#endif // GPS_ENABLE

//--------------------------------------------------------------------------------------------------
/**
 * Handle changes in the AirVantage session state
 *
 * When the session is started the timer to sample the sensors is started and when the session is
 * stopped so is the timer.
 */
//--------------------------------------------------------------------------------------------------
static void AvSessionStateHandler
(
    le_avdata_SessionState_t state,
    void *context
)
{
    switch (state)
    {
        case LE_AVDATA_SESSION_STARTED:
        {
            le_result_t status = le_timer_Start(SampleTimer);
            if (status == LE_BUSY)
            {
                LE_INFO("Received session started when timer was already running");
            }
            else
            {
                LE_ASSERT_OK(status);
            }
            break;
        }

        case LE_AVDATA_SESSION_STOPPED:
        {
            le_result_t status = le_timer_Stop(SampleTimer);
            if (status != LE_OK)
            {
                LE_DEBUG("Record push timer not running");
            }

            break;
        }
        default:
            LE_ERROR("Unsupported AV session state %d", state);
            break;
    }
}

COMPONENT_INIT
{
    SampleTimer = le_timer_Create("Sensor Read");
    LE_ASSERT_OK(le_timer_SetMsInterval(SampleTimer, DelayBetweenReadings * 1000));
    LE_ASSERT_OK(le_timer_SetRepeat(SampleTimer, 1));
    LE_ASSERT_OK(le_timer_SetHandler(SampleTimer, SampleTimerHandler));

    HandlerRef = le_avdata_AddSessionStateHandler(AvSessionStateHandler, NULL);
    AvSession = le_avdata_RequestSession();
    LE_FATAL_IF(AvSession == NULL, "Failed to request avdata session");
}
