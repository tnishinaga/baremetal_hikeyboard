#!/bin/sh -x

qemu-system-x86_64 -cpu qemu64 \
  -drive if=pflash,format=raw,unit=0,file=/usr/share/ovmf/ovmf_code_x64.bin,readonly=on \
  -drive if=pflash,format=raw,unit=1,file=/usr/share/ovmf/ovmf_vars_x64.bin \
  -hda fat:rw:. -nographic