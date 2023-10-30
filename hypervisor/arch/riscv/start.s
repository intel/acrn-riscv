#include <asm/config.h>

	.text

	.globl _start
_start:
	csrr a0, mhartid
	li t0, 0x0f
	csrw pmpcfg0, t0
	li t0, 0xffffffff
	csrw pmpaddr0, t0

	jal init_mstack
	call reset_mtimer
	csrw mip, 0x0
	li t0, 0x9aa
	csrs mstatus, t0

	call init_mtrap

	li t0, 0xaaa
	csrs mideleg, t0

	li t0, 0xaaa
	csrw mie, t0

	li t0, 0xf500
	csrw medeleg, t0

	csrw mscratch, sp
	la t0, _boot
	csrw mepc, t0
	mret

	.globl _boot
_boot:
	jal init_stack
	bnez a0, secondary
	#call test_swi
	#call init_vmode
	call kernel_init
1:
	la ra, 1b
	ret	

	.globl _vkernel
_vkernel:
	jal init_stack
	#call setup_mmu
	li a0, 0
	csrw sie, a0
	#la a0, _vkernel_msg
	#call printk
	call setup_vtrap
	li a0, 0x100
	csrc sstatus, a0
	la a0, guest
	csrw sepc, a0
	li a0, 0
	sret

secondary:
	lw t0, g_cpus
	addi t0, t0, 1
	sw t0, g_cpus, t1
	call boot_trap
	jal boot_idle
	call start_secondary 

init_stack:
	li sp, ACRN_STACK_START
	li t0, ACRN_STACK_SIZE
	mul t0, a0, t0
	sub sp, sp, t0
	#csrw sscrach, sp
	ret

init_mstack:
	li sp, ACRN_MSTACK_START
	li t0, ACRN_MSTACK_SIZE
	mul t0, a0, t0
	sub sp, sp, t0
	ret

	.globl boot_idle
boot_idle:
	wfi
	ret

	.globl _end_boot
_end_boot:
	nop
	ret

_vkernel_msg:
	.string "i'm _vkernel\n"
