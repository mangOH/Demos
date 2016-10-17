/**
 * @file
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"
#include "killSwitch.h"

#define KEY_RED_LED "industrial.redLed"
#define KEY_GREEN_LED "industrial.greenLed"
#define KEY_FAN "industrial.fan"
#define KEY_KILL_SWITCH "industrial.switch"
#define KEY_OVERHEAT_SWITCH "industrial.overheatSwitch"
#define KEY_POWER_COMMAND "industrial.power"


DemoStateMachine::DemoStateMachine(void)
    : _state(State::STARTUP),
      _overheat(BinaryInput::UNKNOWN),
      _localKillSwitch(BinaryInput::UNKNOWN),
      _remoteKillSwitch(BinaryInput::UNKNOWN),
      _pendingOutput(0)
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
        dataRouter_WriteBoolean(KEY_OVERHEAT_SWITCH, overheat, time(NULL));
        stateCheckRequired = true;
    }

    if (stateCheckRequired)
    {
        this->updateState();
    }
    else
    {
        LE_INFO("CAN read event handled, but no state update required");
    }
}

void DemoStateMachine::updateState(void)
{
    // Make sure that at least one CAN read is complete
    if (this->_overheat != BinaryInput::UNKNOWN &&
        this->_localKillSwitch != BinaryInput::UNKNOWN)
    {
        if (this->_localKillSwitch == BinaryInput::ACTIVE ||
            this->_remoteKillSwitch == BinaryInput::ACTIVE)
        {
            if (this->_state != State::DISABLED)
            {
                this->controlRedLed(true);
                this->controlGreenLed(false);
                this->controlOverheatLed(false);
                this->controlFan(false);
                this->writeOutputs();
                LE_DEBUG("Changing state: %d->%d", this->_state, State::DISABLED);
                this->_state = State::DISABLED;
            }
        }
        else if (this->_overheat == BinaryInput::ACTIVE)
        {
            if (this->_state != State::OPERATING_HOT)
            {
                this->controlRedLed(false);
                this->controlGreenLed(true);
                this->controlOverheatLed(true);
                this->controlFan(true);
                this->writeOutputs();
                LE_DEBUG("Changing state: %d->%d", this->_state, State::OPERATING_HOT);
                this->_state = State::OPERATING_HOT;
            }
        }
        else
        {
            if (this->_state != State::OPERATING_NORMAL)
            {
                this->controlRedLed(false);
                this->controlGreenLed(true);
                this->controlOverheatLed(false);
                this->controlFan(false);
                this->writeOutputs();
                LE_DEBUG("Changing state: %d->%d", this->_state, State::OPERATING_NORMAL);
                this->_state = State::OPERATING_NORMAL;
            }
        }
    }
}

void DemoStateMachine::controlRedLed(bool on)
{
    const uint8_t bit = 1 << static_cast<int>(OutputPin::LED_RED);
    if (on)
    {
        this->_pendingOutput |= bit;
    }
    else
    {
        this->_pendingOutput &= ~bit;
    }
    dataRouter_WriteBoolean(KEY_RED_LED, on, time(NULL));
}

void DemoStateMachine::controlGreenLed(bool on)
{
    const uint8_t bit = 1 << static_cast<int>(OutputPin::LED_GREEN);
    if (on)
    {
        this->_pendingOutput |= bit;
    }
    else
    {
        this->_pendingOutput &= ~bit;
    }
    dataRouter_WriteBoolean(KEY_GREEN_LED, on, time(NULL));
}

void DemoStateMachine::controlOverheatLed(bool on)
{
    const uint8_t bit = 1 << static_cast<int>(OutputPin::LED_OVERHEAT);
    if (on)
    {
        this->_pendingOutput |= bit;
    }
    else
    {
        this->_pendingOutput &= ~bit;
    }
    // no data router write for overheat LED
}

void DemoStateMachine::controlFan(bool on)
{
    const uint8_t bit = 1 << static_cast<int>(OutputPin::FAN);
    if (on)
    {
        this->_pendingOutput |= bit;
    }
    else
    {
        this->_pendingOutput &= ~bit;
    }
    dataRouter_WriteBoolean(KEY_FAN, on, time(NULL));
}

void DemoStateMachine::writeOutputs(void)
{
    LE_DEBUG("Writing outputs as 0x%02X", this->_pendingOutput);
    mangoh_canOpenIox1_DigitalOutput_DO0_DO7(this->_pendingOutput);
}

static void timerHandler(le_timer_Ref_t timer)
{
    uint8_t inputs[2];
    inputs[0] = mangoh_canOpenIox1_DigitalInput_DI0_DI7();
    inputs[1] = mangoh_canOpenIox1_DigitalInput_DI8_DI15();
    const uint16_t inputs16 = inputs[0] | (inputs[1] << 8);

    const bool killSwitchOn = (((inputs16 >> static_cast<int>(InputPin::KILL_SWITCH)) & 1) == 0);
    const bool overheat = ((inputs16 >> static_cast<int>(InputPin::OVERHEAT)) & 1);

    LE_DEBUG("timerHandler read inputs as 0x%04X", inputs16);

    DemoStateMachine* stateMachine = static_cast<DemoStateMachine*>(le_timer_GetContextPtr(timer));
    stateMachine->handleEventCanRead(killSwitchOn, overheat);
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
    mangoh_canOpenIox1_ConnectService();
    dataRouter_SessionStart("eu.airvantage.net", "SwiBridge", true, DATAROUTER_CACHE);
    LE_FATAL_IF(mangoh_canOpenIox1_Init() != LE_OK, "Couldn't initialize CAN");

    auto stateMachine = new DemoStateMachine();
    // Create a 1 second timer that repeats infinitely for polling the CAN inputs
    le_timer_Ref_t tr = le_timer_Create("killSwitch");
    LE_ASSERT(le_timer_SetHandler(tr, &timerHandler) == LE_OK);
    LE_ASSERT(le_timer_SetContextPtr(tr, stateMachine) == LE_OK);
    LE_ASSERT(le_timer_SetMsInterval(tr, 1000) == LE_OK);
    LE_ASSERT(le_timer_SetRepeat(tr, 0) == LE_OK);
    LE_ASSERT(le_timer_Start(tr) == LE_OK);

    dataRouter_AddDataUpdateHandler(KEY_POWER_COMMAND, powerUpdateHandler, stateMachine);
}
