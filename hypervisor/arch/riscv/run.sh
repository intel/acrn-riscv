#!/bin/bash

set -x

#qemu-system-riscv32 -machine virt -bios none -kernel hc.elf -s -S -serial pty
#qemu-system-riscv32 -bios none -kernel hc.bin -s -S
#qemu-system-riscv64 -smp 4 -machine virt -bios none -kernel hc.elf -s -S -serial pty
qemu-system-riscv64 -smp 4 -M virt -bios none -kernel hc.elf -s -S -serial pty -display none
#qemu-system-riscv64 -cpu rv64,x-h=true -smp 4 -machine virt -bios none -kernel hc.elf -s -S -serial pty
