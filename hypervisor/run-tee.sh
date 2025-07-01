#!/bin/bash

set -x
set -e

DEFAULT_QEMU=~/qemu/install/bin/qemu-system-riscv64
#DEFAULT_QEMU=qemu-system-riscv64

if [[ x"${QEMU}" = x"" ]]; then
    QEMU=${DEFAULT_QEMU}
fi

${QEMU} -smp 5 -bios build/acrn.elf -gdb tcp::1235 -S -M virt -m 4G,slots=3,maxmem=8G -kernel ~/linux-riscv/vmlinux -device loader,file=./initrd,addr=0x89000000 -device loader,file=./tee.bin,addr=0xC1000000 -nographic
#${QEMU} -smp 5 -cpu max -bios build/acrn.elf -gdb tcp::1235 -S -M virt -m 4G,slots=3,maxmem=8G -kernel ./vmlinux.sos -initrd ./initrd -device loader,file=./tee.bin,addr=0xC1000000 -nographic
#${QEMU} -smp 5 -bios build/acrn.elf -gdb tcp::1235 -S -M virt -m 4G,slots=3,maxmem=8G -kernel ./vmlinux.sos -initrd ./initrd -nographic
