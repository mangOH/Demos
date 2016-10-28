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
#define KEY_BATTERY_READING      "a.wr.read.battery"

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
    double batteryRead;

    const le_result_t result = le_adc_ReadValue("EXT_ADC1", &value);

    if (result == LE_OK)
    {
        LE_INFO("EXT_ADC1 value is: %d", value);
        if (value > 1600)
        {
            value = 1600;
        }

        batteryRead =(((double)(int32_t)value/1600)*100);
        
        const int32_t now = time(NULL);
        dataRouter_WriteFloat(KEY_BATTERY_READING, batteryRead, now);

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
    LE_INFO("---------------------- Battery Reading started");
    
    dataRouter_SessionStart("", "", false, DATAROUTER_CACHE);

    le_timer_Ref_t adcTimerRef = le_timer_Create("ADC Timer");
    le_timer_SetMsInterval(adcTimerRef, ADC_SAMPLE_INTERVAL_IN_MILLISECONDS);
    le_timer_SetRepeat(adcTimerRef, 0);

    le_timer_SetHandler(adcTimerRef, AdcTimer);

    le_timer_Start(adcTimerRef);
}
