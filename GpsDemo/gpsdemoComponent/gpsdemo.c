
//--------------------------------------------------------------------------------------------------
/** @file gpsdemo.c
 * This app reads the current gps location every 5 seconds and sends it to the data router
 *
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * GPS timeout interval in minutes
 *
 * @note Please change this timeout value as needed.
 */
//--------------------------------------------------------------------------------------------------

#define GPSTIMEOUT 1

#define KEY_GPS_LATITUDE      "sensors.mangoh.gps.latitude"
#define KEY_GPS_LONGITUDE     "sensors.mangoh.gps.longitude"

//#define PINNACLE_HOTEL_LATITUDE  4928790
//#define PINNACLE_HOTEL_LONGITUDE -123120900

static const int PINNACLE_HOTEL_LONGITUDE = -123120900;
static const int PINNACLE_HOTEL_LATITUDE = 4928790;

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
static le_result_t GetCurrentLocation(
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


//--------------------------------------------------------------------------------------------------
/**
 * Send the device location to Data Router
 *
 *
 * @note
 *      No failure notification is provided if location services is  unsuccessful.
 */
//--------------------------------------------------------------------------------------------------

static void gpsTimer(
   le_timer_Ref_t gpsTimerRef
)
{
    int32_t latitude;
    int32_t longitude;
    int32_t horizontalAccuracy;

    
    const le_result_t result =
        GetCurrentLocation(&latitude, &longitude, &horizontalAccuracy, GPSTIMEOUT * 60);

    if (result == LE_OK)
    {
        LE_INFO ("real location is :%d,%d", latitude, longitude);
        dataRouter_WriteInteger(KEY_GPS_LATITUDE, latitude, time(NULL));
        dataRouter_WriteInteger(KEY_GPS_LONGITUDE, longitude, time(NULL));

    }
    else
    {
        LE_INFO( "Loc:unknown");
        LE_INFO( " Pinnacle Hotel location is : %d %d", PINNACLE_HOTEL_LATITUDE, PINNACLE_HOTEL_LONGITUDE);
        dataRouter_WriteInteger(KEY_GPS_LATITUDE, PINNACLE_HOTEL_LATITUDE, time(NULL));
        dataRouter_WriteInteger(KEY_GPS_LONGITUDE, PINNACLE_HOTEL_LONGITUDE, time(NULL));


    }
}


COMPONENT_INIT
{
   
    dataRouter_SessionStart("", "", false, DATAROUTER_CACHE);

    le_clk_Time_t timerInterval = { .sec = 60, .usec = 0};
    le_timer_Ref_t gpsTimerRef;
    gpsTimerRef = le_timer_Create("Gps Timer");
    le_timer_SetInterval(gpsTimerRef, timerInterval);
    le_timer_SetContextPtr(gpsTimerRef, NULL);
    le_timer_SetRepeat(gpsTimerRef, 0);
    le_timer_SetHandler(gpsTimerRef, gpsTimer);
    le_timer_Start(gpsTimerRef);
} 
