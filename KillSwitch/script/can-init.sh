#!/bin/sh

export PATH=$PATH:/sbin
modprobe can &&
sleep 3 &&
modprobe can-dev &&
sleep 3 &&
modprobe can-raw &&
sleep 3 &&
modprobe mcp251x &&
sleep 3

CAN=`ifconfig | cut -c -4 | sed 's/^\s\+//' | sort | uniq | grep can0`
if [ -z "$CAN" ]; then
    ip link set can0 type can bitrate 125000 triple-sampling on &&
    sleep 3 &&
    ifconfig can0 up
fi
