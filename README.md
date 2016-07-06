mangOH Demos
============

This repository contains source code for mangOH demos.


Container Tracking
------------------
Simulates tracking of a shipping container using a mangOH Green.  The
demonstration makes use of a SensorTag bluetooth device which integrates a
number of sensors.  The demo supports alarm generation and pushing logged data
to AirVantage web services.

### How To Run
1. Build bluetoothUtil and bleSensorInterface apps by running ```make``` in the
   app directories.
1. Run ```instapp bleSensorInterface.wp85.update 192.168.2.2``` and ```instapp
   bluetoothUtil.wp85.update 192.168.2.2``` to install the two apps on the
   mangOH.
1. Press the power button on the SensorTag device.  The green LED should start
   blinking.
1. Use ```hcitool lescan``` to determine the MAC address of the SensorTag
   device to be used in the demo.
1. On the mangOH, run ```config set bleSensorInterface:sensorMac <MAC
   Address>``` and enter the  MAC address discovered in the previous step.  The
   MAC is stored in persistent storage, so this step only needs to be run again
   if a different SensorTag is to be used.
1. On the mangOH Run ```app start bluetoothUtil```.  This app will launch the
   bleSensorInterface app automatically.


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
