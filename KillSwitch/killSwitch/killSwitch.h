/**
 * @file
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. Use of this work is subject to license.
 */

#ifndef KILL_SWITCH_H
#define KILL_SWITCH_H

enum class OutputPin
{
    LED_GREEN = 0,
    LED_RED = 1,
    LED_OVERHEAT = 2,
    FAN = 3,
};

enum class InputPin
{
    KILL_SWITCH = 0,
    OVERHEAT  = 1,
};

enum class BinaryInput
{
    INACTIVE,
    ACTIVE,
    UNKNOWN,
};
enum class State
{
    STARTUP,
    DISABLED,
    OPERATING_NORMAL,
    OPERATING_HOT,
};

class DemoStateMachine
{
public:
    DemoStateMachine(void);
    void handleEventRemoteKillSwitch(bool killSwitchOn);
    void handleEventCanRead(bool localKillSwitchOn, bool overheatOn);

private:
    void updateState(void);
    void controlRedLed(bool on);
    void controlGreenLed(bool on);
    void controlOverheatLed(bool on);
    void controlFan(bool on);
    void writeOutputs(void);

    State _state;
    BinaryInput _overheat;
    BinaryInput _localKillSwitch;
    BinaryInput _remoteKillSwitch;
    uint8_t _pendingOutput;
};


#endif // KILL_SWITCH_H

