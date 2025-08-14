# ACRN - RISCV64

## Overview
The aim of the project is to enable [ACRN][ACRN] for the [RISCV64][RISCV64] architecture. This project is currently under development and is not yet ready for production. Once this support is implemented and has sufficient quality, it is intended for this port to become a part of the upstream ACRN project and to continue further development there.

The RV-ACRN hypervisor has two working modes:
1) m-ACRN running in M-mode with mem virtualization based on PMP and vcpu context switch in pure software way. m-ACRN is targeted to run on legacy RISCV64 machine without H-ext.
2) h-ACRN running in HS-mode with full H-ext support. h-ACRN is targeted to run on RISCV64 machine with H-ext.

The project is presented at RISC-V summit China 2023.

## Current State

The current development is based on QEMU RV64 virtual platform. The ACRN hypervisor has bootup successfully to ACRN console with the support of SMP, MMU, CLINT & PLIC. The basic virtualization support includes VM lifecycle management, vCPU context switch, memory virtualization via stage-2 page-table translation, IO virtualization via MMIO emulation, as well as IRQ virtualization with CLINT & PLIC emulation.  

Besides, m-ACRN has been verified on both RV64-QEMU virtual platform and TH1520 based hardware platform. Such configurations can run well as Duo-Linux-VMs, Linux SOS + Android UOS, and etc.

h-ACRN is just tested on RV64-QEMU with a built-in KTEST kernel for unit test purpose. Linux VM support is in the TODO list.

Contact haicheng.li@intel.com for any further questions.

## License
This project is released under the terms of [BSD-3-Clause license](LICENSE).

## How To Build & Debug

### acrn-riscv
*	$git clone https://github.com/intel/acrn-riscv.git ~/acrn-riscv
*	$cd ~/acrn-riscv/hypervisor
*	$./build.sh

### run & debug
*	$cd ~/acrn-riscv/hypervisor
*	$./run.sh
*	$./gdb.sh

### RV-ACRN + Linux VM + OP-TEE ###

you can try our RV-ACRN based TEE solution with following extra repo

hACRN+Linux+OpTEE:
1.	Linux: https://github.com/intel/linux-riscv/tree/hvp-acrn
2.	OpTEE: https://github.com/haicheng-li/optee_os

mACRN+Linux+OpTEE:
1.	Linux: https://github.com/intel/linux-riscv/tree/hvp-tee
2.	OpTEE: https://github.com/haicheng-li/optee_os


[ACRN]: https://github.com/projectacrn/acrn-hypervisor
[RISCV64]: https://riscv.org/
