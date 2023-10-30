#include <asm/offset.h>
#include <asm/cpu.h>

	.text

	.globl init_vmode
#init_vmode:
	li t0, 0x8000000000
	csrs mstatus, t0
#	csrr t1, 0x602
#	csrr t1, hedeleg
#	li t0, 0x100
	csrs hedeleg, t0
	ret

	.globl init_vmm
init_vmm:
	csrr t1, 0x600
	li t0, 0x200000080
	csrs 0x600, t0
	csrr t1, 0x600
	csrr t1, 0x602
	li t0, 0x100
	csrs 0x602, t0
	csrr t1, 0x602
	ret

init_vmm_buggy:
	csrr t1, hstatus
	li t0, 0x180
	li t0, 0x200000180
	csrw hstatus, t0
	csrr t1, hstatus
	csrc hstatus, t1
	csrr t1, hstatus
	li t0, 0x200000080
	csrs hstatus, t0
	#csrw hstatus, t0
	#csrw 0x600, t0
	csrr t1, hstatus

	csrr t1, 0x602
	csrs 0x602, t0
	csrr t1, hedeleg
	li t0, 0x100
	csrs hedeleg, t0
	li t0, 0x100
	ret

	.globl vmx_vmrun
vmx_vmrun:
	cpu_disable_irq
	cpu_ctx_save
	sd sp, 0(a0)
	addi a0, a0, 0x8
	csrw sscratch, a0
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
	csrw stvec, t6
	ld t6, REG_T6(a0)
	ld a0, REG_A0(a0)
	sret

	.balign 4
	.global vm_exit
vm_exit:
	csrrw a0, sscratch, a0
	sd ra, REG_RA(a0)
	sd sp, REG_SP(a0)
	sd gp, REG_GP(a0)
	sd tp, REG_TP(a0)
	sd t0, REG_T0(a0)
	sd t1, REG_T1(a0)
	sd t2, REG_T2(a0)
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
	csrrw t1, sscratch, a0
	sd t1, REG_A0(a0)
	la t1, strap_handler
	csrw stvec, t1
	ld sp, -0x8(a0)
	cpu_ctx_restore
	ld sp, REG_SP(sp)
	cpu_enable_irq
	li a0, 0
	ret
