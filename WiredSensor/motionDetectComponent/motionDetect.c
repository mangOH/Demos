//--------------------------------------------------------------------------------------------------
/**
 * This app reads the current adc reading every 1 seconds
 */
//--------------------------------------------------------------------------------------------------
#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * The time between publishing ADC location readings
 *
 * @note Please change this timeout value as needed.
 */
//--------------------------------------------------------------------------------------------------
#define ADC_SAMPLE_INTERVAL_IN_MILLISECONDS (1000)

//--------------------------------------------------------------------------------------------------
/**
 * Motion data to publish to Air Vantage
 */
//--------------------------------------------------------------------------------------------------
#define KEY_MOTION_DETECT      "a.wr.read.motion"

//--------------------------------------------------------------------------------------------------
/**
 * Timer handler  will publish the current ADC reading.
 */
//--------------------------------------------------------------------------------------------------
static void AdcTimer
(
    le_timer_Ref_t adcTimerRef
)
{
    int32_t value;
    int8_t detect;

    const le_result_t result = le_adc_ReadValue("EXT_ADC0", &value);

    if (result == LE_OK)
    {
        LE_INFO("EXT_ADC0 value is: %d", value);
        if (value > 700)
        {
            detect = true;
        }
        else
        {
            detect = false;
        }
        const int32_t now = time(NULL);
        dataRouter_WriteBoolean(KEY_MOTION_DETECT, detect, now);

    }
    else
    {
        LE_INFO("Couldn't get ADC value");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Main program starts here
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("---------------------- Motion Reading started");

    dataRouter_SessionStart("", "", false, DATAROUTER_CACHE);

    le_timer_Ref_t adcTimerRef = le_timer_Create("ADC Timer");
    le_timer_SetMsInterval(adcTimerRef, ADC_SAMPLE_INTERVAL_IN_MILLISECONDS);
    le_timer_SetRepeat(adcTimerRef, 0);

    le_timer_SetHandler(adcTimerRef, AdcTimer);

    le_timer_Start(adcTimerRef);
}
