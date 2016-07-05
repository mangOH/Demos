#!/bin/sh

PATH=$PATH:$EXTRA_PATH

echo "PWD is: $PWD"

#stty -F /dev/ttyHS0 crtscts
#sleep 1

modprobe hci_uart
sleep 1
modprobe bluetooth
sleep 1
modprobe rfcomm
sleep 1
modprobe hidp
sleep 1
modprobe bnep
sleep 1

if [ ! -d "/sys/class/gpio/gpio13" ]; then
    echo 13 > /sys/class/gpio/export
fi
echo up > /sys/class/gpio/gpio13/pull
sleep 1

mux 1  # UART1 to IoT slot 0
mux 15 # Take IoT slot 0 out of reset
sleep 1

hciattach /dev/ttyHS0 texas 115200 flow
sleep 1

hciconfig
sleep 1

hciconfig hci0 up
