#!/bin/bash

IFACE=tap0

sudo qemu-system-aarch64 -m 512 -cpu cortex-a57 -M virt \
 -net nic -net tap,ifname=$IFACE,script=./qemu-ifup,downscript=./qemu-ifdown  \
 -bios QEMU_EFI.fd \
 -hda fat:rw:. \
 -nographic

sudo ip link set dev $IFACE down &> /dev/null
sudo ip tuntap del $IFACE mode tap &> /dev/null 