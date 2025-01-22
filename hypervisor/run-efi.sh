#!/bin/bash

set -x
set -e

DEFAULT_QEMU=qemu-system-riscv64

if [[ x"${QEMU}" = x"" ]]; then
    QEMU=${DEFAULT_QEMU}
fi


${QEMU} \
-bios ~/acrn-riscv.git/hypervisor/build/acrn.elf \
-M virt,pflash0=pflash0,pflash1=pflash1,acpi=off \
-m 4096 -smp 5 \
-gdb tcp::1235 -S \
-serial mon:stdio \
-device virtio-gpu-pci -full-screen \
-device qemu-xhci \
-device usb-kbd \
-device virtio-rng-pci \
-blockdev node-name=pflash0,driver=file,read-only=on,filename=/home/haicheng/jinjiang/RISCV_VIRT_CODE.fd \
-blockdev node-name=pflash1,driver=file,filename=/home/haicheng/jinjiang/RISCV_VIRT_VARS.fd \
-netdev user,id=net0 \
-device virtio-net-pci,netdev=net0 \
-drive file=~/jinjiang/ubuntu-24.10-preinstalled-server-riscv64.img,format=raw,if=virtio \
-nographic
