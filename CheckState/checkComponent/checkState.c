#include "legato.h"
#include "interfaces.h"


//-------------------------------------------------------------------------------------------------
/**
 * Set up UART related parameters.
 */
//-------------------------------------------------------------------------------------------------
static int UartFd = -1;
static char *UartPath = "/dev/ttyHSL0";


//-------------------------------------------------------------------------------------------------
/**
 * Write msg to UART with file descriptor UartFd.
 */
//-------------------------------------------------------------------------------------------------
void WriteLineToUart(char *msg)
{
    const size_t count = strlen(msg);
    ssize_t writeRes = write(UartFd, msg, count);
    LE_FATAL_IF(writeRes != count, "Failed UART write");
    writeRes = write(UartFd, "\n", 1);
    LE_FATAL_IF(writeRes != 1, "Failed UART write");
}

//-------------------------------------------------------------------------------------------------
/**
 * Get current temperature.
 */
//-------------------------------------------------------------------------------------------------
void GetTempInfo
(
    char * tempString
)
{
    int32_t paTemp;
    int32_t pcTemp;

    le_temp_SensorRef_t paSensorRef = le_temp_Request("POWER_AMPLIFIER");
    LE_ASSERT(le_temp_GetTemperature(paSensorRef, &paTemp) == LE_OK);

    le_temp_SensorRef_t pcSensorRef = le_temp_Request("POWER_CONTROLLER");
    LE_ASSERT(le_temp_GetTemperature(pcSensorRef, &pcTemp) == LE_OK);

    sprintf(tempString, "PA: %d, PC: %d", paTemp, pcTemp);

}

//-------------------------------------------------------------------------------------------------
/**
 * Change handler for packet switched state.
 */
//-------------------------------------------------------------------------------------------------
static void PacketSwitchedStateChangeHandler
(
    le_mrc_NetRegState_t state,
    void *contextPtr
)
{
    char stateString[64];
    char tempString[32];
    bool isInfo = true;
    switch (state)
    {
        case LE_MRC_REG_HOME:
            strncpy(stateString, "LE_MRC_REG_HOME", NUM_ARRAY_MEMBERS(stateString) - 1);
            break;
        case LE_MRC_REG_ROAMING:
            strncpy(stateString, "LE_MRC_REG_ROAMING", NUM_ARRAY_MEMBERS(stateString) - 1);
            break;
        case LE_MRC_REG_NONE:
            strncpy(stateString, "LE_MRC_REG_NONE", NUM_ARRAY_MEMBERS(stateString) - 1);
            break;
        default:
            isInfo = false;
            sprintf(stateString, "unknown (%d)", state);
            break;
    }

    GetTempInfo(tempString);
    char message[64];
    sprintf(message, "==[PacketSwitchedState: %s | Temp: %s]==", stateString, tempString);
    if (isInfo)
    {
        LE_INFO(message);
    }
    else
    {
        LE_ERROR(message);
    }
    WriteLineToUart(message);
    GetTempInfo(tempString);
    //measuring signal strength - needs to be moved into it's own handler function
    char qualString[128];
    uint32_t quality;
    LE_ASSERT_OK(le_mrc_GetSignalQual(&quality)); //0 = no signal strength, 5 = very good signal strength
    sprintf(qualString, "==[le_mrc_GetSignalQual returns Signal Strength: %d | Temp: %s]==", quality, tempString);
    LE_INFO(qualString);
    WriteLineToUart(qualString);
}

//-------------------------------------------------------------------------------------------------
/**
 * Change handler for RAT.
 */
//-------------------------------------------------------------------------------------------------
static void RATChangeHandler
(
    le_mrc_Rat_t rat,
    void * contextPtr 
)
{
    char ratString[64];
    char tempString[32];
    bool isInfo = true;
    switch(rat)
    {
        case LE_MRC_RAT_CDMA:
            strncpy(ratString, "LE_MRC_RAT_CDMA", NUM_ARRAY_MEMBERS(ratString) - 1);
            break;
        case LE_MRC_RAT_GSM:
            strncpy(ratString, "LE_MRC_RAT_GSM", NUM_ARRAY_MEMBERS(ratString) - 1);
            break;
        case LE_MRC_RAT_UMTS:
            strncpy(ratString, "LE_MRC_RAT_UMTS", NUM_ARRAY_MEMBERS(ratString) - 1);
            break;
        case LE_MRC_RAT_TDSCDMA:
            strncpy(ratString, "LE_MRC_RAT_TDSCDMA", NUM_ARRAY_MEMBERS(ratString) - 1);
            break;
        case LE_MRC_RAT_LTE:
            strncpy(ratString, "LE_MRC_RAT_LTE", NUM_ARRAY_MEMBERS(ratString) - 1);
            break;
        default:
            isInfo = false;
            sprintf(ratString, "unknown (%d)", rat);
            break;
    }

    GetTempInfo(tempString);
    char message[64];
    sprintf(message, "==[RAT state: %s | Temp: %s]==", ratString, tempString);
    if (isInfo)
    {
        LE_INFO(message);
    }
    else
    {
        LE_ERROR(message);
    }
    WriteLineToUart(message);
}

//-------------------------------------------------------------------------------------------------
/**
 * Change handler for new SIM state.
 */
//-------------------------------------------------------------------------------------------------
static void NewSimStateHandler
(
    le_sim_Id_t simId,
    le_sim_States_t simState,
    void * contextPtr 
)
{
    char simIdString[32];
    char simStateString[32];
    char tempString[32];
    bool isInfo_SimId = true;
    bool isInfo_SimState = true;
    switch(simId)
    {
        case LE_SIM_EMBEDDED:
            strncpy(simIdString, "LE_SIM_EMBEDDED", NUM_ARRAY_MEMBERS(simIdString) - 1);
            break;
        case LE_SIM_EXTERNAL_SLOT_1:
            strncpy(simIdString, "LE_SIM_EXTERNAL_SLOT_1", NUM_ARRAY_MEMBERS(simIdString) - 1);
            break;
        case LE_SIM_EXTERNAL_SLOT_2:
            strncpy(simIdString, "LE_SIM_EXTERNAL_SLOT_2", NUM_ARRAY_MEMBERS(simIdString) - 1);
            break;
        case LE_SIM_REMOTE:
            strncpy(simIdString, "LE_SIM_REMOTE", NUM_ARRAY_MEMBERS(simIdString) - 1);
            break;
        default:
            isInfo_SimId = false;
            sprintf(simIdString, "unknown (%d)", simId);
            break;
    }

    switch(simState)
    {
        case LE_SIM_INSERTED:
            strncpy(simStateString, "LE_SIM_INSERTED", NUM_ARRAY_MEMBERS(simStateString) - 1);
            break;
        case LE_SIM_ABSENT:
            strncpy(simStateString, "LE_SIM_ABSENT", NUM_ARRAY_MEMBERS(simStateString) - 1);
            break;
        case LE_SIM_READY:
            strncpy(simStateString, "LE_SIM_READY", NUM_ARRAY_MEMBERS(simStateString) - 1);
            break;
        case LE_SIM_BLOCKED:
            strncpy(simStateString, "LE_SIM_BLOCKED", NUM_ARRAY_MEMBERS(simStateString) - 1);
            break;
        case LE_SIM_BUSY:
            strncpy(simStateString, "LE_SIM_BUSY", NUM_ARRAY_MEMBERS(simStateString) - 1);
            break;
        default:
            isInfo_SimState = false;
            sprintf(simStateString, "unknown (%d)", simState);
            break;
    }

    GetTempInfo(tempString);
    char message[128];
    sprintf(message, "==[SIM (id: %s) changed to state: %s | Temp: %s]==", simIdString, simStateString, tempString);
    if (isInfo_SimId && isInfo_SimState)
    {
        LE_INFO(message);
    }
    else
    {
        LE_ERROR(message);
    }
    WriteLineToUart(message);
}

//-------------------------------------------------------------------------------------------------
/**
 * Change handler for WiFi Access Point state.
 */
//-------------------------------------------------------------------------------------------------
static void WiFiAccessPointChangeHandler
(
    le_wifiAp_Event_t event,
    void *contextPtr
)
{
    char wifiAPString[64];
    char tempString[32];
    bool isInfo = true;
    switch(event)
    {
        case LE_WIFIAP_EVENT_CLIENT_CONNECTED:
            strncpy(wifiAPString, "LE_WIFIAP_EVENT_CLIENT_CONNECTED", NUM_ARRAY_MEMBERS(wifiAPString) - 1);
            break;
        case LE_WIFIAP_EVENT_CLIENT_DISCONNECTED:
            strncpy(wifiAPString, "LE_WIFIAP_EVENT_CLIENT_DISCONNECTED", NUM_ARRAY_MEMBERS(wifiAPString) - 1);
            break;
        default:
            isInfo = false;
            sprintf(wifiAPString, "unknown (%d)", event);
            break;
    }

    GetTempInfo(tempString);
    char message[64];
    sprintf(message, "==[WiFi access point state: %s | Temp: %s]==", wifiAPString, tempString);
    if (isInfo)
    {
        LE_INFO(message);
    }
    else
    {
        LE_ERROR(message);
    }
    WriteLineToUart(message);
}

COMPONENT_INIT
{
    LE_INFO("==[CheckState App started]==");
    UartFd = open(UartPath, O_WRONLY);
    if (UartFd != -1)
    {
        le_mrc_AddPacketSwitchedChangeHandler(PacketSwitchedStateChangeHandler, NULL);
        le_mrc_AddRatChangeHandler(RATChangeHandler, NULL);
        le_sim_AddNewStateHandler(NewSimStateHandler, NULL);
        le_wifiAp_AddNewEventHandler(WiFiAccessPointChangeHandler, NULL);
    }
    else
    {
        LE_ERROR("Cannot open UART: %s", UartPath);
    }
}
