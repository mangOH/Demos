#include "legato.h"
#include "interfaces.h"

COMPONENT_INIT
{
    if (le_bootReason_WasTimer())
    {
        LE_INFO("Boot reason was timer");
    }
    else if (le_bootReason_WasGpio(36))
    {
        LE_INFO("Boot reason was GPIO 36");
    }
    else
    {
        LE_INFO("Boot reason was not timer or GPIO.  Perhaps a normal boot.");
    }

    LE_INFO("Setting 60s boot timer");
    LE_ASSERT_OK(le_ulpm_BootOnTimer(60));

    LE_INFO("Setting boot from GPIO 36 high");
    LE_ASSERT_OK(le_ulpm_BootOnGpio(36, LE_ULPM_GPIO_HIGH));

    LE_ASSERT_OK(le_ulpm_ShutDown());
}
