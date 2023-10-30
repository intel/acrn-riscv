#include <asm/offset.h>
#include <asm/cpu.h>

	.text

	.globl reset_mtimer
reset_mtimer:
	csrr t0, mhartid
	li t1, 8
	mul t0, t0, t1
	li t1, 0x2004000
	add t0, t0, t1
	li t1, 0xffffffffffffffff
	sd t1, 0(t0)
	ret

	.globl init_mtrap
init_mtrap:
	la t0, mtrap_handler
	csrw mtvec, t0
	ret

	.globl boot_trap
boot_trap:
	addi sp, sp, -8
	sd ra, 0(sp)
	la t0, vect_table
	ori t0, t0, 0x1
	csrw stvec, t0
	li t0, 0x222
	csrw sie, t0
	li t0, 0xC0022
	csrw sstatus, t0
	ld ra, 0(sp)
	addi sp, sp, 8
	ret

	.globl init_trap
init_trap:
	addi sp, sp, -16
	sd ra, 0(sp)
	sd t0, 8(sp)
	la t0, trap_msg
	call do_logmsg

	la t0, strap_handler
	csrw stvec, t0

	li t0, 0x222
	csrw sie, t0
	li t0, 0xC0022
	csrw sstatus, t0

	ld t0, 8(sp)
	ld ra, 0(sp)
	addi sp, sp, 16
	ret

	.globl setup_vtrap
setup_vtrap:
	la t0, vstrap_handler
	csrw stvec, t0
	ret

	.globl kernel_init
kernel_init:
	li t0, 0
	csrw sie, t0
	li t0, 0xC0000
	csrw sstatus, t0
	li a1, 0
	li a2, 0
	call start_acrn
	sret

	.balign 4
mtrap_handler:
	csrrw sp, mscratch, sp
	cpu_mctx_save
	csrr a0, mcause
	li a1, 0x8000000000000000
	and a1, a0, a1
	andi a0, a0, 0xff
	beqz a1, mout
	call mint_handler
mout:
	mv a0, sp
	call m_service
	cpu_mctx_restore
	csrrw sp, mscratch, sp
	mret

	.balign 4
	.globl strap_handler
strap_handler:
	csrrw sp, sscratch, sp
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
	csrrw sp, sscratch, sp
	sret

	.balign 4
vstrap_handler:
	li a0, 0
	csrw sip, a0

	#check if it's in VS mode
	#csrr t1, 0x600

	csrr a0, scause
	li a1, 0x8000000000000000
	and a1, a0, a1
	andi a0, a0, 0xff
	beqz a1, vout
	call sint_handler
vout:
	li a0, 0x000
	csrw sstatus, a0
#	la a0, vstrap_msg
#	call printk
	li a0, 0
	ecall


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
	li t1, 0x2000000
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

	.balign 4
trap_msg:
	.string "init_trap\n"

kernel_msg:
	.string "i'm kernel\n"

vkernel_msg:
	.string "i'm vkernel\n"

idle_msg:
	.string "i'm idle\n"

switch_msg:
	.string "switch context\n"

mtrap_msg:
	.string "mtrap handler\n"

strap_msg:
	.string "strap handler\n"

vstrap_msg:
	.string "vstrap handler\n"

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


	.balign 256
vect_mtrap:
	j mtrap_handler

	.balign 256
vect_strap:
	j strap_handler
