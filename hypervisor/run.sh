#!/bin/bash

set -x
set -e

DEFAULT_QEMU=qemu-system-riscv64

if [[ x"${QEMU}" = x"" ]]; then
    QEMU=${DEFAULT_QEMU}
fi

#qemu-system-x86_64 -smp 4 -kernel build/hypervisor/acrn.32.out -gdb tcp::1235 -S -serial pty -machine kernel-irqchip=split -cpu Denverton -m 1G,slots=3,maxmem=4G
#${QEMU} -smp 4 -kernel build/acrn.elf -gdb tcp::1235 -S -serial pty -M virt -m 1G,slots=3,maxmem=4G
#${QEMU} -kernel build/acrn.elf -gdb tcp::1235 -S -serial stdio -M virt -m 1G,slots=3,maxmem=4G -bios none
${QEMU} -smp 4 -kernel build/acrn.elf -gdb tcp::1235 -S -serial stdio -M virt -m 4G,slots=3,maxmem=8G -bios none
