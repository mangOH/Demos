#ifndef KILL_SWITCH_H
#define KILL_SWITCH_H

enum class OutputPin
{
    LED_GREEN = 0,
    LED_RED = 1,
    FAN = 2,
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
    void controlFan(bool on);
    void controlRedLed(bool on);
    void controlGreenLed(bool on);

    State _state;
    BinaryInput _overheat;
    BinaryInput _localKillSwitch;
    BinaryInput _remoteKillSwitch;

};


#endif // KILL_SWITCH_H

