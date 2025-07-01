#!/bin/bash

set -x
set -e

DEFAULT_GDB=gdb-multiarch

if [[ x"${GDB}" = x"" ]]; then
    GDB=${DEFAULT_GDB}
fi

#${GDB} build/hypervisor/acrn.out
#${GDB} -tui build/hypervisor/acrn.out
#${GDB} -tui -x acrn.gdb -s build/acrn.elf
#${GDB} -tui -x acrn.gdb -s vmlinux.boot
${GDB} -tui -x tee.gdb -s tee.elf
