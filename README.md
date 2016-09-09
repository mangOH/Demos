mangOH Demos
============

This repository contains source code for mangOH demos.

Container Tracking
------------------
Simulates tracking of a shipping container using a mangOH Green.  The
demonstration makes use of a SensorTag bluetooth device which integrates a
number of sensors.  The demo supports alarm generation and pushing logged data
to AirVantage web services.

### Prerequisites

* A mangOH Green DV4 board with a WiFi/Bluetooth IoT card installed in IoT slot 0.
* A Linux kernel with Bluetooth support.  See the [Bluetooth WL18xx driver for mangOH wiki
  page](https://github.com/mangOH/mangOH/wiki/Bluetooth-WL18xx-driver-for-mangOH) for instructions
  on how to build this.
* A Texas Instruments SensorTag development kit

### How To Run

1. Modify `mangOH/targetDefs.mangoh` to set `export SDEF_TO_USE =
   $(MANGOH_ROOT)/samples/Demos/containerTracking.sdef`
1. Build the system: run `make wp85` from inside the legato folder.
1. Run `instlegato wp85 192.168.2.2` to install the system update.
1. Press the power button on edge of the SensorTag
1. On the mangOH, run the command `hcitool lescan` to identify the MAC address of the SensorTag
1. Press `ctrl + c` to end scanning
1. On the mangOH, run the command `config set bleSensorInterface:/sensorMac <MAC>` to set the MAC
   address discovered previously
1. On the mangOH run `app start bleSensorInterface` to start the bluetooth app

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
