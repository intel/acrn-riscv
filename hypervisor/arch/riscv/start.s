/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm/config.h>
#include <asm/cpu.h>

	.text

	.globl _start
_start:
	csrr a0, mhartid

	li t0, BSP_CPU_ID
	blt a0, t0, hart_halt

	la t0, fw_dtb
	sd a1, 0(t0)
	la t0, fw_dinfo
	sd a2, 0(t0)
#ifdef RUN_ON_QEMU
	csrr t0, menvcfg
	li t1, 0xa0000000000000f0
	or t0, t0, t1
	csrw menvcfg, t1
#endif
	csrwi mcounteren, 0x7
	csrwi scounteren, 0x7
	jal init_mstack
	call reset_mtimer
	csrw mip, 0x0
#ifndef CONFIG_MACRN
	call init_mtrap
	li t0, 0x0f
	csrw pmpcfg0, t0
	li t0, 0xffffffff
	csrw pmpaddr0, t0

	li t0, 0x9a0
#else
	call boot_trap
	li t0, 0x1900
#endif
	csrs mstatus, t0


	li t0, 0x222
	csrs mideleg, t0

	li t0, 0xaaa
	csrw mie, t0

	li t0, 0xf0f5ff
	csrw medeleg, t0

	csrw mscratch, sp
	la t0, _boot
	csrw mepc, t0
	mret

	.globl _boot
_boot:
#ifndef CONFIG_MACRN
	jal init_stack
#endif
	li t0, BSP_CPU_ID
	bne a0, t0, secondary

	call kernel_init
1:
	la ra, 1b
	ret

hart_halt:
	wfi
	j  hart_halt

secondary:
	lw t0, g_cpus
	addi t0, t0, 1
	sw t0, g_cpus, t1
#ifndef CONFIG_MACRN
	call boot_trap
#endif
	jal boot_idle
	call start_secondary 

init_stack:
	li sp, ACRN_STACK_TOP
	li t0, ACRN_STACK_SIZE
	mul t0, a0, t0
	sub sp, sp, t0
	csrw sscratch, sp
	ret

init_mstack:
	li sp, ACRN_MSTACK_TOP
	li t0, ACRN_MSTACK_SIZE
	mul t0, a0, t0
	sub sp, sp, t0
	csrw mscratch, sp
	ret

	.globl boot_idle
boot_idle:
	wfi
	ret

	.globl _end_boot
_end_boot:
