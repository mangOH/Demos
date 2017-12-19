ShakeNWake
==========

A simple demonstration app that uses the bmi160 accelerometer on the mangOH Red to wakeup the WP
module from ultra-low power mode.  In order to use this application, a resistor mod is required to
the mangOH Red so that the bmi160 interrupt lines are connected to the "wakeable" WP GPIOs.  Run
`app start shakeNWake` and then the system will enter low power mode.  It will remain in low power
mode for 60s or until significant motion is detected.
