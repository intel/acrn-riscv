#!/bin/bash

set -x
set -e

DEFAULT_GDB=riscv64-unknown-linux-gnu-gdb

if [[ x"${GDB}" = x"" ]]; then
    GDB=${DEFAULT_GDB}
fi

#${GDB} build/hypervisor/acrn.out
#${GDB} -tui build/hypervisor/acrn.out
#${GDB} -tui -x acrn.gdb -s build/acrn.elf
#${GDB} -tui -x acrn.gdb -s vmlinux.boot
${GDB} -tui -x acrn.gdb -s vmlinux.sos
