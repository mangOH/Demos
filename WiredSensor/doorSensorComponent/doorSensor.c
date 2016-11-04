//--------------------------------------------------------------------------------------------------
/**
 * This sample app reads state of  IoT1_GPIO1 (gpio25).
 * If state change is detected, it makes corresponding change in state of LED D750.
 * Use this sample to understand how to configure a gpio as an input or output
 * and use call back function.
 */
//--------------------------------------------------------------------------------------------------
#include "legato.h"
#include "interfaces.h"

#define KEY_DOOR_STATE      "a.wr.read.door"


//--------------------------------------------------------------------------------------------------
/**
 * Performs initial configuration of the CF3 gpio (IoT1_GPIO1)
 */
//--------------------------------------------------------------------------------------------------
static void ConfigureSensorGpio
(
    void
)
{
    LE_FATAL_IF(le_sensorGpio_EnablePullDown() != LE_OK,
                "Couldn't configure gpio for door as pull down");

    // Configure IoT1_GPIO1 as input and set its initial value as high
    LE_FATAL_IF(le_sensorGpio_SetInput(LE_SENSORGPIO_ACTIVE_HIGH) != LE_OK,
                "Couldn't configure cf3 gpio as default input high");
}

//--------------------------------------------------------------------------------------------------
/**
 * Door sensor change handler
 */
//--------------------------------------------------------------------------------------------------
static  void DoorSensorChangeHandler
(
    bool state,
    void *ctx
)
{
    const int32_t now = time(NULL);
    dataRouter_WriteBoolean(KEY_DOOR_STATE, state, now);

}

//--------------------------------------------------------------------------------------------------
/**
 * Main program starts here
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("=============== Door sensor application has started");

    dataRouter_SessionStart("", "", false, DATAROUTER_CACHE);

    ConfigureSensorGpio();

    // Fake a transition immediately to publish the current value
    DoorSensorChangeHandler(le_sensorGpio_Read(), NULL);
    le_sensorGpio_AddChangeEventHandler(LE_SENSORGPIO_EDGE_BOTH,
                                        DoorSensorChangeHandler,
                                        NULL,
                                        0);
}
