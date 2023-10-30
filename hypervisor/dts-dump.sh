#!/bin/bash
set -x

qemu-system-riscv64 -machine virt,dumpdtb=qemu-riscv64-virt.dtb
dtc qemu-riscv64-virt.dtb > qemu-riscv64-virt.dts

