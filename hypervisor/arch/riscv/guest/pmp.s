/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#include <asm/offset.h>
#include <asm/cpu.h>

	.text

	.globl vmx_vmrun
vmx_vmrun:
	cpu_disable_mirq
	cpu_mctx_save
	sd sp, 0(a0)
	addi a0, a0, 0x8
	csrw mscratch, a0
	# a0 is also the guest_regs address
	ld ra, REG_RA(a0)
	ld sp, REG_SP(a0)
	ld gp, REG_GP(a0)
	ld tp, REG_TP(a0)
	ld t0, REG_T0(a0)
	ld t1, REG_T1(a0)
	ld t2, REG_T2(a0)
	ld s0, REG_S0(a0)
	ld s1, REG_S1(a0)
	ld a1, REG_A1(a0)
	ld a2, REG_A2(a0)
	ld a3, REG_A3(a0)
	ld a4, REG_A4(a0)
	ld a5, REG_A5(a0)
	ld a6, REG_A6(a0)
	ld a7, REG_A7(a0)
	ld s2, REG_S2(a0)
	ld s3, REG_S3(a0)
	ld s4, REG_S4(a0)
	ld s5, REG_S5(a0)
	ld s6, REG_S6(a0)
	ld s7, REG_S7(a0)
	ld s8, REG_S8(a0)
	ld s9, REG_S9(a0)
	ld s10, REG_S10(a0)
	ld s11, REG_S11(a0)
	ld t3, REG_T3(a0)
	ld t4, REG_T4(a0)
	ld t5, REG_T5(a0)
	/* Set up host instruction pointer on VM Exit */
	la t6, vm_exit;
	csrw mtvec, t6
	ld t6, REG_EPC(a0)
	csrw mepc, t6
	ld t6, REG_STATUS(a0)
	csrw mstatus, t6
	ld t6, REG_TVAL(a0)
	csrw mtval, t6
	ld t6, REG_CAUSE(a0)
	csrw mcause, t6
	#ld t6, REG_HSTATUS(a0)
	#csrw hstatus, t6
	ld t6, REG_T6(a0)
	ld a0, REG_A0(a0)
	cpu_enable_mirq
	mret

	.balign 4
	.global vm_exit
vm_exit:
	csrrw a0, mscratch, a0

/*
 * if MPP is 0x3 (mstatus 0x1800), i.e. it's the host-irq-interrupted vm_exit
 *  without entering guest yet.
 */
	sd t1, REG_T1(a0)
	sd t2, REG_T2(a0)
	csrr t1, mstatus
	li t2, 0x1800
	and t1, t1, t2
	sub t2, t2, t1
	bnez t2, 1f

/*
 * fake a guest context with previous saved data.
 */
	ld t1, REG_EPC(a0)
	csrw mepc, t1
	ld t1, REG_STATUS(a0)
	csrw mstatus, t1
	ld t1, REG_TVAL(a0)
	csrw mtval, t1

1:
	sd ra, REG_RA(a0)
	sd sp, REG_SP(a0)
	sd gp, REG_GP(a0)
	sd tp, REG_TP(a0)
	sd t0, REG_T0(a0)
#	sd t1, REG_T1(a0)
#	sd t2, REG_T2(a0)
	sd s0, REG_S0(a0)
	sd s1, REG_S1(a0)
	sd a1, REG_A1(a0)
	sd a2, REG_A2(a0)
	sd a3, REG_A3(a0)
	sd a4, REG_A4(a0)
	sd a5, REG_A5(a0)
	sd a6, REG_A6(a0)
	sd a7, REG_A7(a0)
	sd s2, REG_S2(a0)
	sd s3, REG_S3(a0)
	sd s4, REG_S4(a0)
	sd s5, REG_S5(a0)
	sd s6, REG_S6(a0)
	sd s7, REG_S7(a0)
	sd s8, REG_S8(a0)
	sd s9, REG_S9(a0)
	sd s10, REG_S10(a0)
	sd s11, REG_S11(a0)
	sd t3, REG_T3(a0)
	sd t4, REG_T4(a0)
	sd t5, REG_T5(a0)
	sd t6, REG_T6(a0)
	csrr t1, mepc
	sd t1, REG_EPC(a0)
	csrr t1, mstatus
	sd t1, REG_STATUS(a0)
	csrr t1, mtval
	sd t1, REG_TVAL(a0)
	csrr t1, mcause
	sd t1, REG_CAUSE(a0)
	#csrr t1, hstatus
	#sd t1, REG_HSTATUS(a0)
	csrrw t1, mscratch, a0
	sd t1, REG_A0(a0)
	la t1, mtrap_handler
	csrw mtvec, t1
	ld sp, -0x8(a0)
	cpu_mctx_restore
	cpu_enable_mirq
	li a0, 0
	ret
