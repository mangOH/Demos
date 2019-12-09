mangOH Demos
============

This repository contains source code for mangOH demos.

Industrial Automation
---------------------
This demo shows the mangOH using a CAN IoT card to control a CAN device with
inputs and outputs.  The device under control provides a "kill switch" which
allows for the machine to be turned off and back on again.  This demo also
provides a remote kill switch through AirVantage web services.

### How To Run
1. Build the killSwitch app by running ```make``` in the KillSwitch directory.
1. Run ```instapp killSwitch.wp85.update 192.168.2.2``` to install the app.
1. On the mangOH, run
   ```/legato/systems/current/apps/killSwitch/read-only/bin/can-init.sh```.
1. On the mangOH, run ```app start killSwitch```
