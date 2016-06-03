#include "legato.h"
#include "interfaces.h"
#include "killSwitch.h"

#define KEY_RED_LED "industrial.redLed"
#define KEY_GREEN_LED "industrial.greenLed"
#define KEY_FAN "industrial.fan"
#define KEY_KILL_SWITCH "industrial.killSwitch"
#define KEY_EVENT_SWITCH "industrial.eventSwitch"
#define KEY_POWER_COMMAND "industrial.power"


DemoStateMachine::DemoStateMachine(void)
    : _state(State::STARTUP),
      _overheat(BinaryInput::UNKNOWN),
      _localKillSwitch(BinaryInput::UNKNOWN),
      _remoteKillSwitch(BinaryInput::UNKNOWN)
{
}

void DemoStateMachine::handleEventRemoteKillSwitch(bool killSwitchOn)
{
    this->_remoteKillSwitch = killSwitchOn ? BinaryInput::ACTIVE : BinaryInput::INACTIVE;
    this->updateState();
}

void DemoStateMachine::handleEventCanRead(bool localKillSwitchIsOn, bool overheat)
{
    bool stateCheckRequired = false;

    BinaryInput newLocalKillSwitch =
        localKillSwitchIsOn ? BinaryInput::ACTIVE : BinaryInput::INACTIVE;
    if (newLocalKillSwitch != this->_localKillSwitch)
    {
        this->_localKillSwitch = newLocalKillSwitch;
        dataRouter_WriteBoolean(KEY_KILL_SWITCH, localKillSwitchIsOn, time(NULL));
        stateCheckRequired = true;
    }

    BinaryInput newOverheat = overheat ? BinaryInput::ACTIVE : BinaryInput::INACTIVE;
    if (newOverheat != this->_overheat)
    {
        this->_overheat = newOverheat;
        dataRouter_WriteBoolean(KEY_EVENT_SWITCH, overheat, time(NULL));
        stateCheckRequired = true;
    }

    if (stateCheckRequired)
    {
        this->updateState();
    }
}

void DemoStateMachine::updateState(void)
{
    // Make sure that at least one CAN read is complete
    if (this->_overheat != BinaryInput::INACTIVE &&
        this->_localKillSwitch != BinaryInput::INACTIVE)
    {
        if (this->_localKillSwitch == BinaryInput::ACTIVE ||
            this->_remoteKillSwitch == BinaryInput::ACTIVE)
        {
            if (this->_state != State::DISABLED)
            {
                this->controlFan(false);
                this->controlRedLed(true);
                this->controlGreenLed(false);
                this->_state = State::DISABLED;
            }
        }
        else if (this->_overheat == BinaryInput::ACTIVE)
        {
            if (this->_state != State::OPERATING_HOT)
            {
                this->controlFan(true);
                this->controlRedLed(false);
                this->controlGreenLed(true);
                this->_state = State::OPERATING_HOT;
            }
        }
        else
        {
            if (this->_state != State::OPERATING_NORMAL)
            {
                this->controlFan(false);
                this->controlRedLed(false);
                this->controlGreenLed(true);
                this->_state = State::OPERATING_NORMAL;
            }
        }
    }
}

void DemoStateMachine::controlFan(bool on)
{
    // TODO: write to can bus
    dataRouter_WriteBoolean(KEY_FAN, on, time(NULL));
}

void DemoStateMachine::controlRedLed(bool on)
{
    // TODO: write to can bus
    dataRouter_WriteBoolean(KEY_RED_LED, on, time(NULL));
}

void DemoStateMachine::controlGreenLed(bool on)
{
    // TODO: write to can bus
    dataRouter_WriteBoolean(KEY_GREEN_LED, on, time(NULL));
}



static void timerHandler(le_timer_Ref_t timer)
{
    uint8_t inputs[2] = {0, 0};
    // TODO: waiting for CAN API
    //le_result_t r = canOpen_ReadInputs(inputs, sizeof(inputs));
    le_result_t r = LE_OK;
    DemoStateMachine* stateMachine = static_cast<DemoStateMachine*>(le_timer_GetContextPtr(timer));
    if (r == LE_OK)
    {
        const uint16_t inputs16 = inputs[0] | (inputs[1] << 8);
        const bool killSwitchOn = ((inputs16 >> static_cast<int>(InputPin::KILL_SWITCH)) & 1);
        const bool overheat = ((inputs16 >> static_cast<int>(InputPin::OVERHEAT)) & 1);
        stateMachine->handleEventCanRead(killSwitchOn, overheat);
    }
    else
    {
        LE_ERROR("Failed to read CAN inputs");
    }
}

static void powerUpdateHandler(
    dataRouter_DataType_t type,
    const char* key,
    void* contextPtr
)
{
    LE_FATAL_IF(
        strcmp(key, KEY_POWER_COMMAND) != 0,
        "power update handler received update for unexpected data key (%s)",
        key);
    LE_FATAL_IF(
        type != DATAROUTER_BOOLEAN,
        "power update handler received update with unexpected type (%d)",
        type);

    bool powerOn;
    uint32_t timestamp;
    dataRouter_ReadBoolean(key, &powerOn, &timestamp);

    DemoStateMachine* stateMachine = static_cast<DemoStateMachine*>(contextPtr);
    stateMachine->handleEventRemoteKillSwitch(!powerOn);
}

COMPONENT_INIT
{
    dataRouter_SessionStart("eu.airvantage.net", "SwiBridge", true, DATAROUTER_CACHE);

    auto stateMachine = new DemoStateMachine();
    // Create a 1 second timer that repeats infinitely for polling the CAN inputs
    le_timer_Ref_t tr = le_timer_Create("killSwitch");
    LE_ASSERT(le_timer_SetHandler(tr, &timerHandler));
    LE_ASSERT(le_timer_SetContextPtr(tr, stateMachine));
    LE_ASSERT(le_timer_SetMsInterval(tr, 1000) == LE_OK);
    LE_ASSERT(le_timer_SetRepeat(tr, 0) == LE_OK);
    LE_ASSERT(le_timer_Start(tr) == LE_OK);

    dataRouter_AddDataUpdateHandler(KEY_POWER_COMMAND, powerUpdateHandler, stateMachine);
}
