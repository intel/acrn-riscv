/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm/offset.h>
#include <asm/cpu.h>

	.text

	.globl reset_mtimer
reset_mtimer:
	csrr t0, mhartid
	li t1, NR_CPUS
	bgt t0, t1, reset_mtimer
	li t1, 8
	mul t0, t0, t1
	li t1, CONFIG_CLINT_TM_BASE
	add t0, t0, t1
	li t1, 0xffffffffffffffff
	sd t1, 0(t0)
	ret

	.globl init_mtrap
init_mtrap:
	addi sp, sp, -8
	sd t0, 8(sp)
	la t0, mtrap_handler
	csrw mtvec, t0
	ld t0, 8(sp)
	addi sp, sp, 8
	ret

	.globl boot_trap
boot_trap:
	addi sp, sp, -8
	sd ra, 0(sp)
	la t0, vect_table
	ori t0, t0, 0x1
	csrw CSR_TVEC, t0
	li t0, 0x222
	csrw CSR_IE, t0
	li t0, 0xC0022
	csrw CSR_STATUS, t0
	ld ra, 0(sp)
	addi sp, sp, 8
	ret

	.globl init_trap
init_trap:
	addi sp, sp, -16
	sd ra, 0(sp)
	sd t0, 8(sp)

	la t0, strap_handler
	csrw stvec, t0

	li t0, 0x222
	csrw sie, t0
	li t0, 0xC0022
	csrs sstatus, t0

	ld t0, 8(sp)
	ld ra, 0(sp)
	addi sp, sp, 16
	ret

	.globl kernel_init
kernel_init:
	call start_acrn
	sret

	.balign 4
	.globl mtrap_handler
mtrap_handler:
	#csrc mstatus, 0x8
#ifndef CONFIG_MACRN
	csrrw sp, mscratch, sp
#endif
	cpu_mctx_save
	csrr a0, mcause
	li a1, 0x8000000000000000
	and a1, a0, a1
	andi a0, a0, 0xff
	beqz a1, mexcept
	call mint_handler
	j mout
mexcept:
	mv a0, sp
	call m_service
mout:
	cpu_mctx_restore
#ifndef CONFIG_MACRN
	csrrw sp, mscratch, sp
#endif
	#csrs mstatus, 0x8
	mret

	.balign 4
	.globl strap_handler
strap_handler:
#	cpu_disable_irq
	#csrrw sp, sscratch, sp
	cpu_ctx_save
	li a0, 0
	csrw sip, a0
	csrr a0, scause
	li a1, 0x8000000000000000
	and a1, a0, a1
	andi a0, a0, 0xff
	beqz a1, sout
	call sint_handler
sout:
	cpu_ctx_restore
#	cpu_enable_irq
	#csrrw sp, sscratch, sp
	sret

boot_sswi_handler:
	csrc sip, 0x2
	sret

boot_expt_handler:
	cpu_mctx_save
	mv a0, sp
	call m_service
	cpu_mctx_restore
	mret
boot_stimer_handler:
boot_sexti_handler:
	sret

boot_mswi_handler:
	cpu_mctx_save
	csrsi mip, 0x2
	csrc mip, 0x8
	csrr t0, mhartid
	li t1, 4
	mul t0, t0, t1
	li t1, CONFIG_CLINT_BASE
	add t0, t0, t1
	li t1, 0
	sw t1, 0(t0)
	cpu_mctx_restore
	mret

boot_mtimer_handler:
	cpu_mctx_save
	jal reset_mtimer
	li t0, 0x20
	csrs mip, t0
	cpu_mctx_restore
	mret

boot_mexti_handler:
	mret

	.balign 256
vect_table:
	.balign 4
vect_rsv0:
	j boot_expt_handler
	.balign 4
vect_sswi:
	j boot_sswi_handler
	.balign 4
vect_rsv2:
	j boot_expt_handler
	.balign 4
vect_mswi:
	j boot_mswi_handler
	.balign 4
vect_rsv4:
	j boot_expt_handler
	.balign 4
vect_stimer:
	j boot_stimer_handler
	.balign 4
vect_rsv6:
	j boot_expt_handler
	.balign 4
vect_mtimer:
	j boot_mtimer_handler
	.balign 4
vect_rsv8:
	j boot_expt_handler
	.balign 4
vect_sexti:
	j boot_sexti_handler
	.balign 4
vect_rsv10:
	j boot_expt_handler
	.balign 4
vect_mexti:
	j boot_mexti_handler
