#!/bin/bash

set -x

#gdb build/hypervisor/acrn.out
#gdb -tui build/hypervisor/acrn.out
#gdb-multiarch -tui -x acrn.gdb -s build/acrn.elf
riscv64-unknown-linux-gnu-gdb -tui -x acrn.gdb -s build/acrn.elf
