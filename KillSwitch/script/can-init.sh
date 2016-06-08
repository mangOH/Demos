mux 5
mux 16
modprobe can
modprobe can-dev
modprobe can-raw
modprobe mcp251x
ip link set can0 type can bitrate 125000 triple-sampling on
ifconfig can0 up
