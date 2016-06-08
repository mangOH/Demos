#!/bin/sh

PATH=$PATH:$EXTRA_PATH

echo "PWD is: $PWD"

#stty -F /dev/ttyHS0 crtscts
#sleep 1

modprobe hci_uart.ko
sleep 1
modprobe bluetooth
sleep 1
modprobe rfcomm
sleep 1
modprobe hidp.ko
sleep 1
modprobe bnep.ko
sleep 1

mount -t aufs -o dirs=/legato/systems/current/apps/bluetoothUtil/read-only/lib=rw:/lib=ro aufs /lib
sleep 1

echo 13 > /sys/class/gpio/export
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

#app start bleSensorInterface

