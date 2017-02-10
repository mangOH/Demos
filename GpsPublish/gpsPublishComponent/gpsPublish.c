/**
 * @file
 *
 * This app reads the current gps location every 60 seconds and sends it to the data router.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * The time between publishing GPS location readings
 *
 * @note Please change this timeout value as needed.
 */
//--------------------------------------------------------------------------------------------------
#define GPS_SAMPLE_INTERVAL_IN_SECONDS (60)

//--------------------------------------------------------------------------------------------------
/**
 * How long to retry sampling the GPS location before publishing a hard-coded location.
 *
 * @note This value should be < GPS_SAMPLE_INTERVAL_IN_SECONDS
 */
//--------------------------------------------------------------------------------------------------
#define GPS_RETRY_PERIOD_IN_SECONDS (59)

#define KEY_GPS_LATITUDE      "a.gps.location.latitude"
#define KEY_GPS_LONGITUDE     "a.gps.location.longitude"

#define CFG_KEY_DEFAULT_LATITUDE "defaultLatitude"
#define CFG_KEY_DEFAULT_LONGITUDE "defaultLongitude"

//#define USE_REAL_GPS_LOCATION

static int32_t DefaultLatitude = 4928790;
static int32_t DefaultLongitude = -123120900;


#ifdef USE_REAL_GPS_LOCATION
//--------------------------------------------------------------------------------------------------
/**
 * Attempts to use the GPS to find the current latitude, longitude and horizontal accuracy within
 * the given timeout constraints.
 *
 * @return
 *      - LE_OK on success
 *      - LE_UNAVAILABLE if positioning services are unavailable
 *      - LE_TIMEOUT if the timeout expires before successfully acquiring the location
 *
 * @note
 *      Blocks until the location has been identified or the timeout has occurred.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetCurrentLocation
(
    int32_t *latitudePtr,           ///< [OUT] latitude of device - set to NULL if not needed
    int32_t *longitudePtr,          ///< [OUT] longitude of device - set to NULL if not needed
    int32_t *horizontalAccuracyPtr, ///< [OUT] horizontal accuracy of device - set to NULL if not
                                    ///< needed
    uint32_t timeoutInSeconds       ///< [IN] duration to attempt to acquire location data before
                                    ///< giving up.  A value of 0 means there is no timeout.
)
{
    le_posCtrl_ActivationRef_t posCtrlRef = le_posCtrl_Request();
    if (!posCtrlRef)
    {
        LE_ERROR("Can't activate the Positioning service");
        return LE_UNAVAILABLE;
    }

    le_result_t result;
    const time_t startTime = time(NULL);
    LE_INFO("Checking GPS position");
    while (true)
    {
        result = le_pos_Get2DLocation(latitudePtr, longitudePtr, horizontalAccuracyPtr);
        if (result == LE_OK)
        {
            break;
        }
        else if (
            (timeoutInSeconds != 0) &&
            (difftime(time(NULL), startTime) > (double)timeoutInSeconds))
        {
            result = LE_TIMEOUT;
            break;
        }
        else
        {
            // Sleep for one second before requesting the location again.
            sleep(1);
        }
    }

    le_posCtrl_Release(posCtrlRef);

    return result;
}
#endif // USE_REAL_GPS_LOCATION

//--------------------------------------------------------------------------------------------------
/**
 * Timer handler that will publish the current GPS location to the data router.
 *
 * @note
 *      If the location cannot be determined, a hard-coded location will be published.
 */
//--------------------------------------------------------------------------------------------------
static void GpsTimer
(
    le_timer_Ref_t gpsTimerRef
)
{
    int32_t latitude;
    int32_t longitude;

#ifdef USE_REAL_GPS_LOCATION
    int32_t horizontalAccuracy;
    const le_result_t result = GetCurrentLocation(
        &latitude, &longitude, &horizontalAccuracy, GPS_RETRY_PERIOD_IN_SECONDS);
    if (result == LE_OK)
    {
        LE_INFO("Location from GPS is: %d, %d", latitude, longitude);
    }
    else
    {
        latitude = DefaultLatitude;
        longitude = DefaultLongitude;
        LE_INFO(
            "Couldn't get GPS location.  Publishing default location: %d, %d",
            latitude,
            longitude);
    }
#else
    latitude = DefaultLatitude;
    longitude = DefaultLongitude;
#endif // USE_REAL_GPS_LOCATION
    const int32_t now = time(NULL);
    dataRouter_WriteInteger(KEY_GPS_LATITUDE, latitude, now);
    dataRouter_WriteInteger(KEY_GPS_LONGITUDE, longitude, now);
}

COMPONENT_INIT
{
    // Try to read default location from config tree
    le_cfg_IteratorRef_t cfgIter = le_cfg_CreateReadTxn("");
    if (le_cfg_GetNodeType(cfgIter, CFG_KEY_DEFAULT_LATITUDE) == LE_CFG_TYPE_INT &&
        le_cfg_GetNodeType(cfgIter, CFG_KEY_DEFAULT_LONGITUDE) == LE_CFG_TYPE_INT)
    {
        DefaultLatitude = le_cfg_GetInt(cfgIter, CFG_KEY_DEFAULT_LATITUDE, 0);
        DefaultLongitude = le_cfg_GetInt(cfgIter, CFG_KEY_DEFAULT_LONGITUDE, 0);
    }
    le_cfg_CancelTxn(cfgIter);

    dataRouter_SessionStart("", "", false, DATAROUTER_CACHE);

    le_clk_Time_t  timerInterval = {.sec = GPS_SAMPLE_INTERVAL_IN_SECONDS, .usec = 0};
    le_timer_Ref_t gpsTimerRef = le_timer_Create("GPS Timer");

    le_timer_SetInterval(gpsTimerRef, timerInterval);
    le_timer_SetRepeat(gpsTimerRef, 0);
    le_timer_SetHandler(gpsTimerRef, GpsTimer);
    // Explicitly call the timer handler so that the app publishes a GPS location immediately
    // instead of waiting for the first timer expiry to occur.
    GpsTimer(gpsTimerRef);
    le_timer_Start(gpsTimerRef);
}
