#!/bin/sh

qemu-system-aarch64 -m 512 -cpu cortex-a57 -M virt -bios /usr/share/ovmf/ovmf_aarch64.bin -nographic -hda fat:rw:.
