#!/bin/sh

IFACE="enp0s25"

# create bridge
ip link add name br0 type bridge
ip link set dev br0 up

# add interface to bridge
ip link set dev $IFACE down
ip link set dev $IFACE promisc on
ip link set dev $IFACE up
ip link set dev $IFACE master br0

# get ip address from dhcp
dhcpcd br0 &
