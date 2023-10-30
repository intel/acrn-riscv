#!/bin/bash

set -x

#qemu-system-x86_64 -smp 4 -kernel build/hypervisor/acrn.32.out -gdb tcp::1235 -S -serial pty -machine kernel-irqchip=split -cpu Denverton -m 1G,slots=3,maxmem=4G
#qemu-system-riscv64 -smp 4 -kernel build/acrn.elf -gdb tcp::1235 -S -serial pty -M virt -m 1G,slots=3,maxmem=4G
#qemu-system-riscv64 -kernel build/acrn.elf -gdb tcp::1235 -S -serial stdio -M virt -m 1G,slots=3,maxmem=4G -bios none
qemu-system-riscv64 -smp 4 -kernel build/acrn.elf -gdb tcp::1235 -S -serial stdio -M virt -m 1G,slots=3,maxmem=4G -bios none
